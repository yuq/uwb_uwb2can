#include <iostream>
#include <cstring>

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <linux/can.h>
#include <linux/sockios.h>
#include <linux/can/raw.h>

#include "can.h"

/* 0 = disabled (default), 1 = enabled */
#define CONFIG_RECV_OWN_MSG  0

Can::Can(std::string candev, bool block)
	: m_canfd(-1), m_block(block), m_can_name(candev)
{
}

Can::~Can()
{
	close_can();
}

bool Can::open_can()
{
	int s;
	struct sockaddr_can addr;
	struct ifreq ifr;

	s = socket(PF_CAN, SOCK_RAW, CAN_RAW);	//open a socket,raw socket protocol,not BCM
	if (s < 0) {
		std::cerr << "open socket fail\n";
		return false;
	}

	if (!m_block) {
		int flags = fcntl(s, F_GETFL, 0);
		if (flags < 0) {
			std::cerr << "get socket flags fail\n";
			goto err0;
		}
		flags |= O_NONBLOCK;
		if (fcntl(s, F_SETFL, flags) < 0) {
			std::cerr << "set socket flags fail\n";
			goto err0;
		}
	}

	strcpy(ifr.ifr_name, m_can_name.c_str());
	if (ioctl(s, SIOCGIFINDEX, &ifr) < 0) {
		std::cerr << "ioctl fail\n";
		goto err0;
	}
	addr.can_family = AF_CAN;
	addr.can_ifindex = ifr.ifr_ifindex;
	if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		std::cerr << "bind fail\n";
		goto err0;
	}

	m_canfd = s;

	std::cout << "Can device open sucess, m_canfd =" << m_canfd << "addr.ifindex =" << addr.can_ifindex << std::endl;

	return true;

err0:
	close(s);
	return false;
}

bool Can::read_can(can_frame *pframe)
{
	int n;

	n = read(m_canfd, pframe, sizeof(struct can_frame));
	if (n < 0) {
        if (n != -1 || errno != EAGAIN)
            std::cerr << "can raw socket read error " << n << std::endl;
		return false;
	} else if (n == 0) {
		// no more data
		return false;
	} else if (n < (int)sizeof(struct can_frame)) {
		std::cerr << "read: incomplete CAN frame\n";
		return false;
	}

	return true;
}

bool Can::write_can(const can_frame *pframe)
{
    int n = write(m_canfd, pframe, sizeof(struct can_frame));
    if(n < 0) {
        std::cerr << "write fail:" << n << std::endl;
		return false;
    } else if (n == 0) {
        // can't write any more
        return false;
    } else if (n < (int)sizeof(struct can_frame)) {
        std::cerr << "write: incomplete CAN frame\n";
        return false;
    }

	return true;
}

void Can::close_can()
{
	if (m_canfd > 0) {
		close(m_canfd);
		m_canfd = -1;
	}
}
