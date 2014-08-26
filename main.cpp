#include <iostream>
#include <cstdio>
#include <cstring>
#include <cmath>

#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "can.h"

struct can_pack {
	int16_t x;
	int16_t y;
	int16_t z;
	int16_t kaopu;
} __attribute__((packed));

int main(void)
{
	Can can("can0");
	if (!can.open_can()) {
		std::cerr << "open can fail\n";
		return 0;
	}

	// socket
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		std::cerr << "open socket fail\n";
		return 0;
	}

	// opt
	int sflag = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char *)&sflag, sizeof(sflag))) {
		std::cerr << "set socket opt fail\n";
		return 0;
	}

	// bind
	sockaddr_storage ss;
	socklen_t ss_len;
	memset(&ss, 0, sizeof(ss));

	sockaddr_in *address = (sockaddr_in *)&ss;
	ss_len = sizeof(sockaddr_in);

	address->sin_family = AF_INET;
	address->sin_addr.s_addr = INADDR_ANY;
	address->sin_port = htons(8123);

	if (bind(fd, (sockaddr*)&ss, ss_len)) {
		std::cerr << "bind socket fail\n";
		return 0;
	}

	while (1) {
		// listen
		if (listen(fd, 3)) {
			std::cerr << "listen socket fail\n";
			break;
		}

		// accept
		struct sockaddr_in addr;
		memset(&addr, 0, sizeof(struct sockaddr_in));
		socklen_t addrlen = sizeof(addr);

		int net = accept(fd, (struct sockaddr*)&addr, &addrlen);
		if (net < 0) {
			std::cerr << "accept socket fail\n";
			continue;
		}
		std::cout << "accept one client\n";

		FILE *file = fdopen(net, "rw");
		if (file == NULL) {
			std::cerr << "fdopen fail\n";
			continue;
		}

		// read out one lines which may be damaged
		char buff[128] = {0};
		char *ret = fgets(buff, 128, file);
		if (ret == NULL) {
			std::cerr << "fgets error\n";
			continue;
		}
		else
			std::cout << "fgets: " << buff << std::endl;

		while (1) {
/*
			char buff[100];
			int n = read(net, buff, 99);
			if (n <= 0) {
				std::cerr << "read fail " << n << std::endl;
				break;
			}
			buff[n] = '\0';
			std::cout << buff;
			continue;
//*/
			int x = 0, y = 0, z = 0;
			int err = fscanf(file, "x=%d y=%d z=%d\n", &x, &y, &z);
			if (err < 0) {
				std::cerr << "read socket fail " << err << std::endl;
				break;
			}
			else if (err == 3) {
				struct can_frame frame;
				struct can_pack *pack = (struct can_pack *)frame.data;
				pack->x = x;
				pack->y = y;
				pack->z = z;
				pack->kaopu = sqrt(x * x + y * y + z * z);
				frame.can_id = 0x523;
				frame.can_dlc = 8;

				std::cout << "send x=" << x << " y=" << y << " z=" << z << std::endl;
				if (!can.write_can(&frame)) {
					std::cerr << "write can fail\n";
					// re-open can
					can.close_can();
					if (!can.open_can()) {
						std::cerr << "re-open can fail\n";
						return 0;
					}
					std::cout << "re-open can\n";
					continue;
				}
				std::cout << "send done\n";
			}
			else {
				std::cerr << "read socket wrong format\n";
				continue;
			}
		}

	}

	return 0;
}




