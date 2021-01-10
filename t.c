#include <stdio.h>
#include <stdint.h>

static uint16_t jbd_crc2(uint8_t *puchMsg, uint8_t usDataLen)
{
  uint8_t lengthData = puchMsg[3];

  uint16_t summa = 0;

  for (int i = 4; i < lengthData + 4; i++)
    summa = summa + puchMsg[i];

  uint16_t checkSum = (summa + lengthData - 1) ^ 0xFFFF;
        printf("checkSum: %x\n", checkSum);
  return checkSum;
}

static uint16_t jbd_crc(unsigned char *data, int len) {
        unsigned short sum = 0;
        register int i;

        printf("len: %d\n", len);
        for(i=0; i < len; i++) {
		printf("data[i]: %x\n", data[i]);
		sum += data[i];
//		sum -= data[i];
		printf("sum: %x\n", sum);
	}
        printf("sum: %x\n", sum);
	sum = (sum ^ 0xFFFF) + 1;
        printf("sum: %x\n", sum);
        return sum;
}

int main(void) {
	unsigned char data[] = { 0xDD,0x18,0x00,0x02,0x0A,0x99,0xFF,0x58,0x77 };
	unsigned short sum;

	jbd_crc(&data[3],sizeof(data)-6);
	jbd_crc2(data,sizeof(data));
	sum = data[6] << 8 | data[7];
	printf("pkt chksum: %x\n", sum);
}
