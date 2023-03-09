# queue  

Persistent queue implementation for ESP32 stored in user specified partition in flash - when no space is available oldest elements are deleted until space is available

Maximum Queue element size is 255 bytes

file: queue.ino - test/play/exmaple  
file: eeprom_queue.h - implementation  
  
IMPORTANT: Before using please update :     
  
#define EEPROM_Q_PARTIOTION_NAME "spiffs"  
#define EEPROM_Q_PARTITION_SIZE (32 * 1024)  
  
with the propriate values for you storage partition name and partions size in bytes  
  
example partion table :  
  
# Name,   Type, SubType, Offset,   Size, Flags  
nvs,      data, nvs,     0x9000,   0x4000,  
phy_init, data, phy,     0xD000,   0x1000,  
otadata,  data, ota,     0xE000,   0x2000,  
ota_0,    app,  ota_0,   0x10000,  1984K,  
ota_1,    app,  ota_1,   0x200000, 1984K,  
eeprom,   data, 0x99,    0x3F0000, 0x1000,  
nvs_key,  data, nvs_keys,0x3F1000, 0x4000  
spiffs,   data, spiffs,  0x3F5000, 32K,  
#12K not used  
  
functions implemented :  
  
inline void init_q() - call before everything else  
inline void erase_q() - erase the whole partiotion  
inline uint32_t get_q_count() - current count of elements in the queue  
inline void get_q_first(uint8_t buf[], uint8_t & size) - read withot remove  
inline void q_first_remove() - remove   
inline void add_q_last(uint8_t buf[], uint8_t size) - add element - if no space delete from beggining until space is available  
  
usage :  
  
init_q  
...  
add_q_last  
...  
add_q_last  
...  
get_q_first  
q_first_remove  
...  
get_q_first  
q_first_remove  
  
dependancy: Log64 lib  ( if not required remove all lines with LOG64_XXXX )  
