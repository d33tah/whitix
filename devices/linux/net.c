#include <console.h>
#include <module.h>

unsigned char* skb_put(void* skb, unsigned int len)
{
	KePrint("skb_put\n");
	return NULL;
}

SYMBOL_EXPORT(skb_put);

/* netif */

void __netif_schedule(void* dev)
{
	KePrint("__netif_schedule\n");
}

SYMBOL_EXPORT(__netif_schedule);

void netif_carrier_off(void* dev)
{
	KePrint("netif_carrier_off\n");
}

SYMBOL_EXPORT(netif_carrier_off);

void netif_carrier_on(void* dev)
{
	KePrint("netif_carrier_on\n");
}

SYMBOL_EXPORT(netif_carrier_on);

void netif_device_attach(void* dev)
{
	KePrint("netif_device_attach\n");
}

SYMBOL_EXPORT(netif_device_attach);

void netif_device_detach(void* dev)
{
	KePrint("netif_device_detach\n");
}

SYMBOL_EXPORT(netif_device_detach);

int netif_receive_skb(void* skb)
{
	KePrint("netif_receive_skb\n");
	return 0;
}

SYMBOL_EXPORT(netif_receive_skb);

/* netdev */
int register_netdev(void* dev)
{
	KePrint("register_netdev(%#X)\n", dev);
	return 0;
}

SYMBOL_EXPORT(register_netdev);

void unregister_netdev(void* dev)
{
	KePrint("unregister_netdev(%#X)\n", dev);
}

SYMBOL_EXPORT(unregister_netdev);

void free_netdev(void* dev)
{
	KePrint("free_netdev\n");
}

SYMBOL_EXPORT(free_netdev);

void __napi_schedule()
{
	KePrint("__napi_schedule\n");
}

SYMBOL_EXPORT(__napi_schedule);

void dev_kfree_skb_any(void* skb)
{
	KePrint("dev_kfree_skb_any\n");
}

SYMBOL_EXPORT(dev_kfree_skb_any);

void* dev_alloc_skb(unsigned int length)
{
	KePrint("dev_alloc_skb\n");
	return NULL;
}

SYMBOL_EXPORT(dev_alloc_skb);
