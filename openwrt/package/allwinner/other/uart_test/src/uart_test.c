#include <stdio.h> /*标准输入输出定义*/
#include <stdlib.h> /*标准函数库定义*/
#include <unistd.h> /*Unix 标准函数定义*/
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h> /*文件控制定义*/
#include <termios.h> /*PPSIX 终端控制定义*/
#include <errno.h> /*错误号定义*/
#include <string.h>
#include <sys/ioctl.h>
#include <linux/serial.h>
#include <sys/time.h>

enum parameter_type {
	PT_PROGRAM_NAME = 0,
	PT_DEV_NAME,
	PT_CYCLE,
	PT_NUM
};

int baudrate = 115200;
int data_size = 16;
int cnt = 100;
int port = 1;
int flow_ctrl = 0;
int debug_flag = 0;
int loopback = 0;
int time_size = 0;
int rtx = 0;
int debug_time_flag = 0;

#define DBG(string, args...) \
do { \
	printf("%s, %s()%d---", __FILE__, __FUNCTION__, __LINE__); \
	printf(string, ##args); \
	printf("\n"); \
} while (0)

static int OpenDev(char *name)
{
	int fd = open(name, O_RDWR ); //| O_NOCTTY | O_NDELAY
	if (-1 == fd)
	DBG("Can't Open(%s)!", name);

	return fd;
}

static int set_opt(int fd,int nSpeed, int nBits, char nEvent, int nStop)
{
	struct termios newtio,oldtio;

	if ( tcgetattr( fd,&oldtio) != 0) {
		perror("SetupSerial 1"); return -1;
	}
	bzero( &newtio, sizeof( newtio ) );
	newtio.c_cflag |= CLOCAL | CREAD; //CLOCAL:忽略modem控制线 CREAD：打开接受者
	newtio.c_cflag &= ~CSIZE; //字符长度掩码。取值为：CS5，CS6，CS7或CS8
	switch( nBits ) {
		case 7: newtio.c_cflag |= CS7; break;
		case 8: newtio.c_cflag |= CS8; break;
	}
	switch( nEvent ) {
		case 'O':
			newtio.c_cflag |= PARENB; //允许输出产生奇偶信息以及输入到奇偶校验
			newtio.c_cflag |= PARODD; //输入和输出是奇及校验
			newtio.c_iflag |= (INPCK | ISTRIP); // INPACK:启用输入奇偶检测；ISTRIP：去掉第八位
		break;
		case 'E':
			newtio.c_iflag |= (INPCK | ISTRIP);
			newtio.c_cflag |= PARENB;
			newtio.c_cflag &= ~PARODD;
		break;
		case 'N':
			newtio.c_cflag &= ~PARENB;
		break;
	}
	printf("nSpeed:%d\n", nSpeed);
	switch( nSpeed ) {
		case 2400:
			cfsetispeed(&newtio, B2400);
			cfsetospeed(&newtio, B2400);
			break;
		case 4800:
			cfsetispeed(&newtio, B4800);
			cfsetospeed(&newtio, B4800);
			break;
		case 9600:
			cfsetispeed(&newtio, B9600);
			cfsetospeed(&newtio, B9600);
			break;
		case 115200:
			cfsetispeed(&newtio, B115200);
			cfsetospeed(&newtio, B115200);
			break;
		case 460800:
			cfsetispeed(&newtio, B460800);
			cfsetospeed(&newtio, B460800);
			break;
		case 1000000:
			cfsetispeed(&newtio, B1000000);
			cfsetospeed(&newtio, B1000000);
			break;
		case 1500000:
			cfsetispeed(&newtio, B1500000);
			cfsetospeed(&newtio, B1500000);
			break;
		default: {
			#if 1
			struct serial_struct ss;
			cfsetispeed(&newtio, B38400);
			cfsetospeed(&newtio, B38400);
			tcflush(fd, TCIFLUSH);
			tcsetattr(fd, TCSANOW, &newtio);
			if ((ioctl(fd, TIOCGSERIAL, &ss) < 0))
				printf("-----ioctl failed\n");
			ss.flags = ASYNC_SPD_CUST;
			ss.custom_divisor = ss.baud_base/nSpeed;
			/*ss.custom_divisor = ss.baud_base/330000;*/
			printf("baud base:%u\n", ss.baud_base);
			if ((ioctl(fd, TIOCSSERIAL, &ss) < 0))
				printf("-----ioctl failed\n");
			#endif
		}
		break;
	}
	if( nStop == 1 )
		newtio.c_cflag &= ~CSTOPB; //CSTOPB:设置两个停止位，而不是一个
	else if ( nStop == 2 )
		newtio.c_cflag |= CSTOPB; newtio.c_cc[VTIME] = 0; //VTIME:非cannoical模式读时的延时，以十分之一秒位单位
	newtio.c_cc[VMIN] = 0; //VMIN:非canonical模式读到最小字符数

	if (flow_ctrl == 1)
		newtio.c_cflag |= CRTSCTS;
	else
		newtio.c_cflag &= ~CRTSCTS;

	tcflush(fd,TCIFLUSH); // 改变在所有写入 fd 引用的对象的输出都被传输后生效，所有已接受但未读入的输入都在改变发生前丢弃。
	if((tcsetattr(fd,TCSANOW,&newtio))!=0) //TCSANOW:改变立即发生
	{
		perror("com set error"); return -1;
	}
	printf("set done!\n\r");
	return 0;
}


static void str_print(char *buf, int len, int r)
{
	int i;

	if (debug_flag == 0)
		return;

	if (r == 1)
		printf("==== read data ====\n");
	else
		printf("==== write data ====\n");

	for (i=0; i<len; i++) {
		if (i%16 == 0)
			printf("\n");
		printf("0x%02x ", buf[i]);
	}
	printf("\n");
}

static void print_usage(const char *prog)
{
	printf("Usage: %s [-dbcsf]\n", prog);
	puts("  -d  uart to test, input 0/1/2... (default uart1)\n"
		 "  -b  baudate (defalut 115200)\n"
		 "  -n  test cycle (default 100 times)\n"
		 "  -s  data size (default 16 Bytes)\n"
		 "  -t  print the transmit information after a few times(default 10 times)\n"
		 "  -f  flow control (default n)\n"
		 "  -l  loopback (default n)\n"
		 "  -g  set to open debug log\n"
		 "  -r  print read fifo log\n"
		 "  -m  change to recive mode(default is trans mode)\n"
		 "  -p  print the speed info (default n)\n"
		 "  -h  print help\n");
	printf("if you want to test urat speed, please set recive mode first\n");
	printf("speed test(trans) eg:uart_test -d 1 -n 100 -t 10 -p\n");
	printf("speed test(recive) eg:uart_test -d 1 -n 100 -t 10 -p -m\n");
	printf("loopback eg:uart_test -d 1 -b 1500000 -n 10 -s 256 -l -g\n");
	exit(1);
}

static void parse_opts(int argc, char *argv[])
{
	while (1) {
		int c;

		c = getopt(argc, argv, "d:b:n:s:t:fpgrhlm");

		if (c == -1)
			break;
		switch (c) {
		case 'd':
			port = atoi(optarg);
			break;
		case 'b':
			baudrate = atoi(optarg);
			break;
		case 'n':
			cnt = atoi(optarg);
			break;
		case 's':
			data_size = atoi(optarg);
			break;
		case 't':
			time_size = atoi(optarg);
			break;
		case 'f':
			flow_ctrl = 1;
			break;
		case 'p':
			debug_time_flag = 1;
			break;
		case 'g':
			debug_flag = 1;
			break;
		case 'l':
			loopback = 1;
			break;
		case 'h':
			print_usage(argv[0]);
			break;
		case 'm':
			rtx = 1;
			break;
		default:
			print_usage(argv[0]);
			break;
		}
	}
}

int main(int argc, char *argv[])
{
	struct timeval dly_tm, start, end;
	int i = 0, err_cnt = 0, suc_cnt = 0, io_cnt = 0, ret = 0, fd = 0, temp = 0, fd_loop, x;
	int sec = 0, usec = 0, final_sec = 0, final_usec = 0, ave_time = 0;
	int write_size = 0, read_size = 0;
	char *read_buf, *write_buf;
	char buf[256];
	fd_set rd_fdset;

	char uart_port[16] = {0};

	parse_opts(argc, argv);

	sprintf(uart_port, "/dev/ttyS%d", port);

	printf("=======%s test args=======\n", argv[0]);
	printf("uart_port: %s.\n", uart_port);
	printf("baudrate: %d.\n", baudrate);
	printf("test cycle: %d.\n", cnt);
	printf("bytes_per_cycle: %d.\n", data_size);
	printf("flow ctrl:%s.\n", flow_ctrl?"true":"false");
	printf("debug log:%s.\n", debug_flag?"true":"false");
	printf("%s\n", rtx?"recive mode":"trans mode");
	printf("\n");

	write_buf = malloc(data_size + 1);
	if(NULL == write_buf) {
		printf("malloc for write_buf error\n");
		return -1;
	}

	read_buf = malloc(data_size + 1);
	if(NULL == read_buf) {
		printf("malloc for read_buf error\n");
		free(write_buf);
		write_buf = NULL;
		return -1;
	}
	memset(read_buf, 0 , data_size + 1);

	fd = OpenDev(uart_port);
	if (fd < 0) {
		printf("open uart%d failed\n", port);
		goto help;
	}

	set_opt(fd, baudrate, 8, 'N', 1);

	printf("Select(%s), Cnt %d. \n", uart_port, cnt);
	char loopback_port[64];
	if (loopback == 1) {
		sprintf(loopback_port, "/sys/devices/platform/soc/uart%d/loopback", port);
		fd_loop = OpenDev(loopback_port);
		write(fd_loop, "enable", 7);
	}

	i = 0;

	printf("[%s] line:%d read_size=%d\n", __func__, __LINE__, read_size);
	if (!rtx) {
		while (i < cnt) {
			tcflush(fd,TCIOFLUSH);
			gettimeofday(&start, NULL);
			/*sprintf(write_buf, "%ld", start.tv_usec);*/
			/*for (x = 0;x < data_size - 1; write_buf[x++] = rand());*/
			for (x = 0; x < data_size - 1; x++)
				write_buf[x] = x;
			write_size = write(fd, write_buf, data_size);
			if (debug_flag == 1)
				printf("%d:transmit %d bytes.\n", i, write_size);
			str_print(write_buf, write_size, 0);
			usleep(200000);

			FD_ZERO(&rd_fdset);
			FD_SET(fd, &rd_fdset);

			dly_tm.tv_sec = 10;
			dly_tm.tv_usec = 0;

			i++;
			ret = select(fd+1, &rd_fdset, NULL, NULL, &dly_tm);
			if (ret == 0) {
				printf("%d:select timeout\n", i);
				err_cnt++;
				continue;
			}
			if (ret < 0) {
				printf("%d:select(%s) return %d. [%d]: %s \n", i, uart_port, ret, errno,
						strerror(errno));
				err_cnt++;
				continue;
			}
			memset(read_buf, 0, data_size + 1);
			read_size = read(fd, read_buf, data_size);
			gettimeofday(&end, NULL);
			tcflush(fd,TCIFLUSH);
			read_size = 0;

			if(memcmp(read_buf, write_buf, data_size) != 0)
				err_cnt++;

			if (debug_time_flag == 1) {
				sec += end.tv_sec - start.tv_sec;
				usec += end.tv_usec - start.tv_usec;
				io_cnt++;
				if (io_cnt % time_size == 0) {
					suc_cnt = io_cnt - err_cnt;
					printf("cnt:  %d\n", io_cnt);
					printf("err_cnt:  %d\n", err_cnt);
					printf("suc_cnt:  %d\n", suc_cnt);
					printf("time:  %ds %dus\n", sec, usec);
					final_sec += sec;
					final_usec += usec;
					sec = 0;
					usec = 0;
					io_cnt = 0;
					err_cnt = 0;
				}
				if (cnt == i)
					printf("The test finally spend: %ds %dus\n", final_sec, final_usec);
			}
		}
	} else {
		while (1) {
			FD_ZERO(&rd_fdset);
			FD_SET(fd, &rd_fdset);

			dly_tm.tv_sec = 10;
			dly_tm.tv_usec = 0;

			i++;
			ret = select(fd+1, &rd_fdset, NULL, NULL, &dly_tm);
			if (ret == 0) {
				printf("%d:select timeout\n", i);
				if (i > 10) {
					printf("error, test failed\n");
					break;
				}
				continue;
			}
			if (ret < 0) {
				printf("%d:select(%s) return %d. [%d]: %s \n", i, uart_port, ret, errno,
						strerror(errno));
				if (i > 10) {
					printf("error, test failed\n");
					break;
				}
				continue;
			}
			memset(read_buf, 0, data_size + 1);
			read_size = read(fd, read_buf, data_size);
			tcflush(fd,TCIFLUSH);
			read_size = 0;

			tcflush(fd,TCIOFLUSH);
			write_size = write(fd, read_buf, data_size);
			if (debug_flag == 1)
				printf("%d:transmit %d bytes.\n", i, write_size);
			str_print(read_buf, write_size, 0);
		}
	}
	printf("test complete\n");
	free(read_buf);
	free(write_buf);
	close(fd);
	if (loopback == 1)
		close(fd_loop);
	return err_cnt;
help:
	print_usage(argv[0]);
	return -1;
}
