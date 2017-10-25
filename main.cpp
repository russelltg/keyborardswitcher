#include <cassert>
#include <iostream>
#include <fstream>
#include <chrono>

#include <xcb/xcb.h>
#include <xcb/xinput.h>

enum button {
	lhs = 0b1,
	mbs = 0b1 << 1,
	rhs = 0b1 << 2,
	all3 = lhs | mbs | rhs
};

const char* attachScript = R"(virsh --connect qemu:///system attach-device win10 /tmp/virshkeyboard.xml --live)";

const char* detachScript = R"(virsh --connect qemu:///system detach-device win10 /tmp/virshkeyboard.xml --live)";


const char* xmlFile = R"(
<hostdev mode='subsystem' type='usb' managed='yes' >
	<source>
		<vendor  id='0x046d' />
		<product id='0xc31c' />
	</source>
</hostdev>
)";

using namespace std::string_literals;

int main() {
	// get a X connection
	auto conn = xcb_connect(nullptr, nullptr);
	assert(conn != nullptr);
	
	// get an X screen
	auto iter = xcb_setup_roots_iterator(xcb_get_setup(conn));
	xcb_screen_t* screen = nullptr;
	for (int i = 0; iter.rem; xcb_screen_next(&iter), ++i) {
		std::cout << "Screen " << i << ": " << iter.data->width_in_pixels << "x" << iter.data->height_in_pixels << std::endl;
		screen = iter.data;
	}
	assert(screen != nullptr);
	
	
	uint32_t     values[2] = {screen->white_pixel,
                                    XCB_EVENT_MASK_BUTTON_PRESS   |
                                    XCB_EVENT_MASK_BUTTON_RELEASE };

	struct {xcb_input_event_mask_t head; xcb_input_xi_event_mask_t mask; } mask;
	mask.head.deviceid = 12;
	mask.head.mask_len = sizeof(mask.mask) / sizeof(uint32_t);
	
	mask.mask = xcb_input_xi_event_mask_t(XCB_INPUT_XI_EVENT_MASK_BUTTON_PRESS | XCB_INPUT_XI_EVENT_MASK_BUTTON_RELEASE);
	
	xcb_input_xi_select_events(conn, screen->root, 1, &mask.head);
	
	xcb_flush(conn);
	
	uint8_t pressedbuttons = 0;
	std::chrono::system_clock::time_point startTime;
	
	std::chrono::system_clock::time_point::duration timeForAction = std::chrono::milliseconds(100);
	bool attached = false;
	
	xcb_generic_event_t* e{};
	while((e = xcb_wait_for_event(conn))) {
		if ((e->response_type & ~80) == XCB_GE_GENERIC) {
			switch(((xcb_ge_event_t*)e)->event_type) {
			case XCB_INPUT_BUTTON_PRESS: {
				
				auto ev = (xcb_input_button_press_event_t*)e;
				
				if ((pressedbuttons & all3) == all3) {
					break;
				}
				if (ev->detail == 1) {
					pressedbuttons |= lhs;
				}
				else if (ev->detail == 2) {
					pressedbuttons |= mbs;
				}
				else if (ev->detail == 3) {
					pressedbuttons |= rhs;
				}
				
				if ((pressedbuttons & all3) == all3) {
					startTime = std::chrono::system_clock::now();
				}
				
				break;
			}
			case XCB_INPUT_BUTTON_RELEASE: {
				
				auto ev = (xcb_input_button_release_event_t*)e;
				
				bool allPressed = (pressedbuttons & all3) == all3;
				
				if (ev->detail == 1) {
					pressedbuttons &= ~lhs;
				}
				if (ev->detail == 2) {
					pressedbuttons &= ~mbs;
				}
				if (ev->detail == 3) {
					pressedbuttons &= ~rhs;
				}
				
				if (allPressed && !((pressedbuttons & all3) == all3)) {
					auto duration = std::chrono::system_clock::now() - startTime;
					
					if (duration >= timeForAction) {
						// trigger action
						
						// save XML file
						{
							std::ofstream stream{"/tmp/virshkeyboard.xml"};
							stream << xmlFile;
						}
						
						// see if we attach or detach
						if (attached) {
							std::cout << "Detaching Keyboard..." << std::endl;
							
							std::string command = "sh -c '"s + detachScript + "'";
							system(command.c_str());
						} else {
							std::cout << "Attaching Keyboard..." << std::endl;
							
							std::string command = "sh -c '"s + attachScript + "'";
							system(command.c_str());
						}
						attached = !attached;
					}
				}
				
				break;
			}
			}
		}
		free(e);
	}
}
