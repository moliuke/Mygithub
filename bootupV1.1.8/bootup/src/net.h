#ifndef __NET_H
#define __NET_H
void net_Init(void);
int net_ConnectServer(void);
void net_CloseSocket(void);
int net_Send(char *tx_buf,int len);
int net_Recv(char *rx_buf,int len);

#endif

