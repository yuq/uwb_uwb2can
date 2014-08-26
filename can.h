#ifndef _CAN_H_
#define _CAN_H_

#include <string>
#include <linux/can.h>

class Can
{
public:
	Can(std::string candev, bool block = true);
	~Can();

	bool open_can();
	bool read_can(struct can_frame *pframe);
    bool write_can(const struct can_frame *pframe);
	void close_can();
    int get_fd() { return m_canfd; }

protected:
	int m_canfd;
	bool m_block;
	std::string m_can_name;
};

#endif // _CAN_H_
