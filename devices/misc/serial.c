#include <i386/ioports.h>
#include <fs/devfs.h>
#include <fs/vfs.h>
#include <module.h>
#include <print.h>
#include <typedefs.h>
#include <devices/device.h>
#include <error.h>
#include <devices/class.h>
#include <malloc.h>
#include <task.h>

#define BASE_BAUD		(1843200 / 16)

#define STD_COM_FLAGS	0

#define SERIAL_SEND_BUF_SIZE	4096

/* Types */
#define PORT_UNKNOWN	0
#define PORT_16450		1
#define PORT_16550A		2
#define PORT_8250		3

struct SerialStruct
{
	unsigned long uart;
	unsigned short port;
	unsigned short irq;
	unsigned long flags;
};

struct SerialStruct serials[]=
{
	{BASE_BAUD, 0x3F8, 4, STD_COM_FLAGS},
	{BASE_BAUD, 0x2F8, 3, STD_COM_FLAGS},
	{BASE_BAUD, 0x3E8, 4, STD_COM_FLAGS},
	{BASE_BAUD, 0x2E8, 3, STD_COM_FLAGS},
};

struct SerialDev
{
	struct KeDevice device;
	struct SerialStruct* info;
	int count;
	int timeout;
	BYTE type, mcr, ier;
	BYTE xmitFifoSize;
	
	/* Buffer transmission */
	char* sendBuffer;
	int sendCnt, sendHead, sendTail;
};

struct SerialDev serialDevs[4];

/* Registers. */

/* In */
#define UART_RX		0x00
#define UART_IIR	0x02
#define UART_LSR	0x05
#define UART_MSR	0x06

/* Out */
#define UART_TX		0x00
#define UART_DLL	0x00
#define UART_DLM	0x01
#define UART_IER	0x01
#define UART_FCR	0x02
#define UART_LCR	0x03
#define UART_MCR	0x04

#define UART_SCR	0x07

/* Line control register */
#define UART_LCR_WLEN8		0x03

/* Line status register. */
#define UART_LSR_TEMT		0x40
#define UART_LSR_THRE		0x20
#define UART_LSR_DR			0x01

/* Modem control register */
#define UART_MCR_LOOP		0x10
#define UART_MCR_OUT2		0x08
#define UART_MCR_OUT1		0x04
#define UART_MCR_RTS		0x02
#define UART_MCR_DTR		0x01

/* Interrupt enable register. */
#define UART_IER_MSI		0x08
#define UART_IER_RLSI		0x04
#define UART_IER_THRI		0x02
#define UART_IER_RDI		0x01

/* FIFO control register defines. */
#define UART_FCR_ENABLE_FIFO	0x01
#define UART_FCR_CLEAR_RCVR		0x02
#define	UART_FCR_CLEAR_XMIT		0x04
#define UART_FCR_DMA_SELECT		0x08
#define UART_FCR_TRIGGER_MASK	0xC0
#define UART_FCR_TRIGGER_1		0x00
#define UART_FCR_TRIGGER_4		0x40
#define UART_FCR_TRIGGER_8		0x80
#define UART_FCR_TRIGGER_14		0xC0

/* Latch control register. */
#define UART_LCR_DLAB			0x80

/* Interrupt identification register. */
#define UART_IIR_NO_INT			0x01
#define UART_IIR_ID				0x02

static inline BYTE SerialIn(struct SerialStruct* info, WORD port)
{
	BYTE ret;
	
	ret = inb(info->port + port);
	inb(0x80);
	return ret;
}
#define SerialOut(info, addr, val) 	do { outb((info)->port + addr, (val)); inb(0x80); } while(0)

void SerialAutoConfig(struct SerialDev* dev, struct SerialStruct* info)
{
	DWORD flags;
	BYTE scratch, scratch2, status1, status2;
	
	if (!info->port)
		return;
		
	IrqSaveFlags(flags);
	
	scratch = SerialIn(info, UART_IER);
	SerialOut(info, UART_IER, 0);
	
	outb(0x80, 0xFF);
	
	scratch2 = SerialIn(info, UART_IER);
	SerialOut(info, UART_IER, scratch);
	
	if (scratch2)
		/* Failed. */
		goto out;
		
	SerialOut(info, UART_FCR, UART_FCR_ENABLE_FIFO);
	scratch = SerialIn(info, UART_IIR) >> 6;
		
	dev->xmitFifoSize = 1;
	
	switch (scratch)
	{
		case 0:
			dev->type = PORT_16450;
			break;
			
		case 3:
			dev->type = PORT_16550A;
			dev->xmitFifoSize = 16;
			break;
			
		default:
			KePrint("serial = %d\n", scratch);
	}
	
	if (dev->type == PORT_16450)
	{
		scratch = SerialIn(info, UART_SCR);
		SerialOut(info, UART_SCR, 0xA5);
		status1 = SerialIn(info, UART_SCR);
		SerialOut(info, UART_SCR, 0x5A);
		status2 = SerialIn(info, UART_SCR);
		
		if ((status1 != 0xA5) || (status2 != 0x5A))
			dev->type = PORT_8250;
	}
	
	/* Reset the UART */
	SerialOut(info, UART_MCR, 0x00);
	SerialOut(info, UART_FCR, (UART_FCR_CLEAR_RCVR | UART_FCR_CLEAR_XMIT));
	SerialIn(info, UART_RX);
			
out:
	IrqRestoreFlags(flags);
}

static void SerialSend(struct SerialDev* dev)
{
	int i;
	
	if (!dev->sendCnt)
	{
		dev->ier &= ~UART_IER_THRI;
		SerialOut(dev->info, UART_IER, dev->ier);
		return;
	}
	
	for (i=0; i<dev->xmitFifoSize; i++)
	{
		SerialOut(dev->info, UART_TX, dev->sendBuffer[dev->sendTail++]);
		dev->sendTail &= (SERIAL_SEND_BUF_SIZE-1);
		
		if (!--dev->sendCnt)
			break;	
	}
	
	if (!dev->sendCnt)
	{
		dev->ier &= ~UART_IER_THRI;
		SerialOut(dev->info, UART_IER, dev->ier);
	}
}

static int SerialInterrupt(void* data)
{
	BYTE status;
	struct SerialDev* dev = (struct SerialDev*)data;
	int passCount = 0;
	
	/* Check IIR here? qemu's serial driver doesn't seem to like that. */
	
	do
	{
		status = SerialIn(dev->info, UART_LSR);
		
		if (status & UART_LSR_DR)
		{
			KePrint("Recv\n");
		}
		
		if (passCount++ > 64)
			break;
	
		if (status & UART_LSR_THRE)
			SerialSend(dev);
	
	}while (!(SerialIn(dev->info, UART_IIR) & UART_IIR_NO_INT));
	
	return 0;
}

struct SerialDev* FileToSerialDev(struct File* file)
{
	struct KeDevice* device;
	
	device = DevFsGetDevice(file->vNode);
	
	if (!device)
		return NULL;
		
	return ContainerOf(device, struct SerialDev, device);
}

void SerialChangeSpeed(struct SerialDev* dev, int speed)
{
	int quot;
	BYTE fcr, cval;
	
	cval = 0160 >> 4;
	
	quot = dev->info->uart / speed;
	dev->timeout = (dev->xmitFifoSize*100*15/speed)+2;
	
	dev->mcr |= UART_MCR_DTR;
	
	SerialOut(dev->info, UART_MCR, dev->mcr);
	
	if (dev->type == PORT_16550A)
	{
		if (speed < 2400)
			fcr = UART_FCR_ENABLE_FIFO | UART_FCR_TRIGGER_1;
		else
			fcr = UART_FCR_ENABLE_FIFO | UART_FCR_TRIGGER_8;
	}else
		fcr = 0;
	
	SerialOut(dev->info, UART_LCR, UART_LCR_DLAB | cval);
	SerialOut(dev->info, UART_DLL, quot & 0xFF);
	SerialOut(dev->info, UART_DLM, quot >> 8);
	SerialOut(dev->info, UART_LCR, cval);
	SerialOut(dev->info, UART_FCR, fcr);
}

int SerialStartup(struct SerialDev* dev)
{
	DWORD flags;
	
	if (!dev->sendBuffer)
	{
		dev->sendBuffer = (char*)MemAlloc(SERIAL_SEND_BUF_SIZE);
		
		if (!dev->sendBuffer)
			return -ENOMEM;
	}
	
	IrqSaveFlags(flags);
	
	if (dev->type == PORT_16550A)
	{
		SerialOut(dev->info, UART_FCR, (UART_FCR_CLEAR_RCVR | UART_FCR_CLEAR_XMIT));
		dev->xmitFifoSize = 16;
	}else
		dev->xmitFifoSize = 1;
		
	/* TODO: Check LSR? */
	
	/* Clear the interrupt registers. */
	SerialIn(dev->info, UART_RX);
	SerialIn(dev->info, UART_IIR);
	SerialIn(dev->info, UART_MSR);
	
	SerialOut(dev->info, UART_LCR, UART_LCR_WLEN8);
	
	dev->mcr = UART_MCR_DTR | UART_MCR_RTS | UART_MCR_OUT2;
	
	SerialOut(dev->info, UART_MCR, dev->mcr);
	
	/* Register interrupt */
	IrqAdd(dev->info->irq, SerialInterrupt, dev);
	
	dev->ier = UART_IER_MSI | UART_IER_RLSI | UART_IER_RDI;
	SerialOut(dev->info, UART_IER, dev->ier);
	
	/* And clear the interrupt register again. */
	SerialIn(dev->info, UART_LSR);
	SerialIn(dev->info, UART_RX);
	SerialIn(dev->info, UART_IIR);
	SerialIn(dev->info, UART_MSR);
	
	SerialChangeSpeed(dev, 9600);
	
	IrqRestoreFlags(flags);
	
	return 0;
}

int SerialDevOpen(struct File* file)
{
	struct SerialDev* serialDev;
	int err;
	
	serialDev = FileToSerialDev(file);
	
	if (!serialDev)
		return -ENOENT;
	
	serialDev->count++;
		
	if (serialDev->count == 1)
	{
		err = SerialStartup(serialDev);
		if (err)
			return err;
	}
	
	return 0;
}

int SerialDevRead(struct File* file, BYTE* data, DWORD size)
{
	KePrint("SerialDevRead\n");
	return 0;
}

int SerialDevWrite(struct SerialDev* dev, BYTE* data, DWORD size)
{
	DWORD flags;
	int total;
	
	if (!dev || !dev->sendBuffer)
		return 0;
		
	IrqSaveFlags(flags);
		
	while (1)
	{
		int c;
		
		c = MIN(size, MIN(SERIAL_SEND_BUF_SIZE - dev->sendCnt - 1,
				SERIAL_SEND_BUF_SIZE - dev->sendHead));
				
		if (!c)
			break;
			
		memcpy(dev->sendBuffer + dev->sendHead, data, c);
		dev->sendHead = (dev->sendHead + c) & (SERIAL_SEND_BUF_SIZE -1);
		dev->sendCnt += c;
		data += c;
		size -= c;
		total += c;
	}
	
	if (dev->sendCnt > 0 && !(dev->ier & UART_IER_THRI))
	{
		dev->ier |= UART_IER_THRI;
		SerialOut(dev->info, UART_IER, dev->ier);
	}
}

int SerialFileWrite(struct File* file, BYTE* data, DWORD size)
{
	return SerialDevWrite(FileToSerialDev(file), data, size);
}

int SerialDevClose(struct File* file)
{
	KePrint("SerialDevClose\n");
	return 0;
}

struct FileOps serialOps={
	.open = SerialDevOpen,
	.read = SerialDevRead,
	.write = SerialFileWrite,
	.close = SerialDevClose,
};

/* IcFs */
struct IcAttribute serialAttributes[]=
{
	IC_END(),
};

KE_OBJECT_TYPE_SIMPLE(serialType, NULL);

struct DevClass serialClass;

SYMBOL_EXPORT(serialClass);

void SerialWrite(char* str, int len)
{
	return SerialDevWrite(&serialDevs[0], str, len);
}

int SerialInit()
{
	unsigned int i;
	int err;
	
	err = DevClassCreate(&serialClass, &serialType, "Serials");
	
//	SerialStartup(&serialDevs[0]);
//	serialDevs[0].count = 1;
//	KeSetOutput(SerialWrite);
	
	if (err)
	{
		KePrint(KERN_ERROR "SER: Could not register serial device class.\n");
		return err;
	}
	
	KePrint(KERN_INFO "SER: Scanning for serial ports\n");
	
	for (i = 0; i < count(serials); i++)
	{
		SerialAutoConfig(&serialDevs[i], &serials[i]);
		
		if (serialDevs[i].type == PORT_UNKNOWN)
			continue;
			
		KePrint(KERN_INFO "SER: Serial%d at %#X is a ",
			i, serials[i].port, serials[i].irq);

		/* Set up the device structure. */
		serialDevs[i].info = &serials[i];

		KeDeviceCreate(&serialDevs[i].device, &serialClass.set,
			DEV_ID_MAKE(0, i), &serialOps, DEVICE_CHAR, "Serial%d", i);
		
		switch (serialDevs[i].type)
		{
			case PORT_8250:
				KePrint("8250\n");
				break;
				
			case PORT_16450:
				KePrint("16450\n");
				break;
				
			case PORT_16550A:
				KePrint("16550A\n");
				break;
		}
	}
	
	return 0;
}

ModuleInit(SerialInit);
