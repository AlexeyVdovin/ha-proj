#ifndef _PACKET_IO_H_
#define _PACKET_IO_H_

static uchar rx_pkt[sizeof(packet_t)+MAX_DATA_LEN+2];
static uchar _rx_pos = 0;
static long rx_time = 0;

static packet_t* rx_packet()
{
    packet_t *pkt = (packet_t*)rx_pkt;
    register uchar rx_pos = _rx_pos;

    // Check for timeout    
    if(rx_pos && rx_time && get_time() > rx_time) rx_pos = 0;

    while(rx_count())
    {
        int c = rx_byte();
        if(c < 0) break;
        register uchar u = (uchar)(c & 0x00FF);
        
        if(rx_pos == 0 || (rx_pos == 1 && u == DATA_ID1))
        {
            rx_pos = 0;
            if(u != DATA_ID1) continue;
            else rx_time = get_time()+PACKET_RX_TIMEOUT;
        }
        if(rx_pos == 1 && u != DATA_ID2) { rx_pos = 0; continue; }
        if(rx_pos == sizeof(packet_t)-1 && pkt->len > MAX_DATA_LEN) { rx_pos = 0; continue; }
        if(rx_pos < sizeof(packet_t) || rx_pos < sizeof(packet_t) + pkt->len + 2)
        { 
            rx_pkt[rx_pos++] = u;
        }
        if(rx_pos >= sizeof(packet_t) + pkt->len + 2)
        {
            rx_pos = 0;
            ushort* crc = (ushort*)(pkt->data + pkt->len);
            if(pkt->len > 0 && *crc == packet_crc(pkt)) { _rx_pos = rx_pos; return pkt; }
        }
    }
    _rx_pos = rx_pos;
    return NULL;
}

static void tx_packet(packet_t* pkt)
{
    uchar i = pkt->len + sizeof(packet_t)+2, *c = (uchar*)pkt;
    ushort* crc = (ushort*)(pkt->data + pkt->len);
    *crc = packet_crc(pkt);
    while(--i) tx_byte(*c++);
}

#endif /* _PACKET_IO_H_ */

