#include <console.h>
#include <module.h>
#include <print.h>

#include "include/net.h"
#include "include/module.h"

int mii_ethtool_sset(void* mii, void* ecmd)
{
	KePrint("mii_ethtool_sset\n");
	return 0;
}

SYMBOL_EXPORT(mii_ethtool_sset);

int mii_ethtool_gset(void* mii, void* ecmd)
{
	KePrint("mii_ethtool_gset\n");
	return 0;
}

SYMBOL_EXPORT(mii_ethtool_gset);

int mii_link_ok(void* mii)
{
	KePrint("mii_link_ok\n");
	return 0;
}

SYMBOL_EXPORT(mii_link_ok);

int mii_nway_restart(void* mii)
{
	KePrint("mii_nway_restart\n");
	return 0;
}

SYMBOL_EXPORT(mii_nway_restart);

int generic_mii_ioctl(void* inf, void* data, int cmd, unsigned int* chg_out)
{
	KePrint("generic_mii_ioctl\n");
	return 0;
}

SYMBOL_EXPORT(generic_mii_ioctl);

unsigned short eth_type_trans(void* skb, void* dev)
{
	KePrint("eth_type_trans\n");
	return 0;
}

SYMBOL_EXPORT(eth_type_trans);

quickcall void print_mac(char mac[6])
{
	int i;
	
	for (i=0; i<6; i++)
		KePrint("%2.2X%c", mac[i], (i == 5) ? '\n' : ':');
}

SYMBOL_EXPORT(print_mac);

void crc32_le(unsigned long crc, char* p, unsigned long len)
{
	KePrint("crc32_le\n");
}

SYMBOL_EXPORT(crc32_le);

quickcall struct net_device* alloc_etherdev_mq(int priv, unsigned int queue_count)
{
	struct net_device* ret = (struct net_device*)MemAlloc(sizeof(struct net_device));
	
	return ret;
}

SYMBOL_EXPORT(alloc_etherdev_mq);
