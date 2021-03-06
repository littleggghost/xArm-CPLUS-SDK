/* Copyright 2017 UFACTORY Inc. All Rights Reserved.
 *
 * Software License Agreement (BSD License)
 *
 * Author: Jimy Zhang <jimy92@163.com>
 ============================================================================*/
 //#include <unistd.h>
#ifdef _WIN32
#include <windows.h>
#include<io.h>
#else
#include <unistd.h>
#endif
#include "xarm/core/instruction/uxbus_cmd_ser.h"
#include "xarm/core/common/crc16.h"
#include "xarm/core/debug/debug_print.h"
#include "xarm/core/instruction/uxbus_cmd_config.h"

UxbusCmdSer::UxbusCmdSer(SerialPort *arm_port) { arm_port_ = arm_port; }
UxbusCmdSer::~UxbusCmdSer(void) {}

int UxbusCmdSer::check_xbus_prot(unsigned char *datas, int funcode) {
	if (datas[3] & 0x40)
	{
		return UXBUS_STATE::ERR_CODE;
	}
	else
		if (datas[3] & 0x20)
		{
			return UXBUS_STATE::WAR_CODE;
		}
		else
		{
			return 0;
		}
}

int UxbusCmdSer::send_pend(int funcode, int num, int timeout, unsigned char *ret_data) {
	int ret;
	// unsigned char rx_data[arm_port_->que_maxlen_] = {0};
	unsigned char *rx_data = new unsigned char[arm_port_->que_maxlen_];
	int times = timeout;
	while (times) {
		times -= 1;
		ret = arm_port_->read_frame(rx_data);
		if (ret != -1) {
			ret = check_xbus_prot(rx_data, funcode);
			for (int i = 0; i < num; i++) { ret_data[i] = rx_data[i + 4]; }
			delete rx_data;
			return ret;
		}
		//usleep(1000);
#ifdef _WIN32
		Sleep(1); // 1 ms
#else
		usleep(1000); // 1000us
#endif
	}
	delete rx_data;
	return UXBUS_STATE::ERR_TOUT;
}

int UxbusCmdSer::send_xbus(int funcode, unsigned char *datas, int num) {
	int i;
	// unsigned char send_data[num + 4];
	unsigned char *send_data = new unsigned char[num + 4];

	send_data[0] = UXBUS_CONF::MASTER_ID;
	send_data[1] = UXBUS_CONF::SLAVE_ID;
	send_data[2] = num + 1;
	send_data[3] = funcode;
	for (i = 0; i < num; i++) { send_data[4 + i] = datas[i]; }

	int crc = modbus_crc(send_data, 4 + num);
	send_data[4 + num] = (unsigned char)(crc & 0xFF);
	send_data[5 + num] = (unsigned char)((crc >> 8) & 0xFF);

	arm_port_->flush();
	int ret = arm_port_->write_frame(send_data, num + 6);
	delete send_data;
	return ret;
}

void UxbusCmdSer::close(void) { arm_port_->close_port(); }
