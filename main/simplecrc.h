#ifndef __SIMPLECRC_H__
#define __SIMPLECRC_H__  

uint32_t crc32_for_byte(uint32_t r);
void crc32(const void *data, size_t n_bytes, uint32_t* crc);

#endif
