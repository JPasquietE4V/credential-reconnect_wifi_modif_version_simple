#ifndef __SIMPLECRC_H__
#define __SIMPLECRC_H__  

uint32_t crc32_for_byte(uint32_t r);
void crc32(const char* p_ssid, const char* p_pass, uint32_t *crc);

#endif
