/**
   USE OF THIS SOFTWARE IS GOVERNED BY THE TERMS AND CONDITIONS
   OF THE LICENSE STATEMENT AND LIMITED WARRANTY FURNISHED WITH
   THE PRODUCT.
   <p/>
   IN PARTICULAR, YOU WILL INDEMNIFY AND HOLD B2N LTD., ITS
   RELATED COMPANIES AND ITS SUPPLIERS, HARMLESS FROM AND AGAINST ANY
   CLAIMS OR LIABILITIES ARISING OUT OF THE USE, REPRODUCTION, OR
   DISTRIBUTION OF YOUR PROGRAMS, INCLUDING ANY CLAIMS OR LIABILITIES
   ARISING OUT OF OR RESULTING FROM THE USE, MODIFICATION, OR
   DISTRIBUTION OF PROGRAMS OR FILES CREATED FROM, BASED ON, AND/OR
   DERIVED FROM THIS SOURCE CODE FILE.
*/

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ring Q implementation with elements stored in Eeprom
//
// functions available :
//
//inline void init_q()
//inline void erase_q()
//inline uint32_t get_q_count()
//inline void get_q_first(uint8_t buf[], uint8_t & size)
//inline void q_first_remove()
//inline void add_q_last(uint8_t buf[], uint8_t size)
//
// usage
//
// init_q
// ...
// add_q_last
// ...
// add_q_last
// ...
// get_q_first
// q_first_remove
// ...
// get_q_first
// q_first_remove

// 2 blocks for definitions - rolling 4 x 4 bytes  - start , end of Q, element count in Q, start pose for delete  -  valid one is before first  12 bytes with FF, or last 12 bytes from the block
// data is in format <size(1 byte), data (size bytes)> - rolling q
// TABLE[(first,last,count,del).....] DATA[....del....fist....last....] when one blick 4K between del and the first - erase and ready for reuse

#include <esp_partition.h>

// change depending on partition table
#define EEPROM_Q_PARTIOTION_NAME "spiffs"
#define EEPROM_Q_PARTITION_SIZE (32 * 1024)

#define EEPROM_Q_BLOCK_SIZE (4 * 1024)

// change if bigger table is required ( to save erase cycles)
#define EEPROM_Q_TABLE_SIZE (EEPROM_Q_BLOCK_SIZE * 2)

#define EEPROM_Q_DATA_START (EEPROM_Q_TABLE_SIZE)
#define EEPROM_Q_DATA_SIZE (EEPROM_Q_PARTITION_SIZE - EEPROM_Q_TABLE_SIZE)

#define EEPROM_Q_DUMP \
  LOG64_SET(F("FIRST[")); \
  LOG64_SET((uint32_t)first); \
  LOG64_SET(F("] LAST[")); \
  LOG64_SET((uint32_t)last); \
  LOG64_SET(F("] COUNT[")); \
  LOG64_SET((uint32_t)count); \
  LOG64_SET(F("] DEL[")); \
  LOG64_SET((uint32_t)del); \
  LOG64_SET(F("] "));


esp_partition_t *eeprom_q_partition = NULL;

// cached values - as else the q is too slow to find first valid record in the table
uint32_t eeprom_q_first = 0xFFFFFFFF;
uint32_t eeprom_q_last = 0xFFFFFFFF;
uint32_t eeprom_q_count = 0xFFFFFFFF;
uint32_t eeprom_q_del = 0xFFFFFFFF;

uint32_t eeprom_q_table_pos = 0xFFFFFFFF;

inline void eeprom_q_read(uint32_t p, uint8_t buf[], uint32_t size)
{

  esp_err_t err = esp_partition_read(eeprom_q_partition, p, buf, size);
  if ( err != ESP_OK)
  {
    LOG64_SET(F("EEPROM Q: READ ERR["));
    LOG64_SET((uint32_t)err);
    LOG64_SET(F("]"));
    LOG64_SET((uint32_t)p);
    LOG64_SET((uint32_t)size);
    LOG64_NEW_LINE;
  }

}

inline void eeprom_q_write(uint32_t p, uint8_t buf[], uint32_t size)
{

  esp_err_t err = esp_partition_write(eeprom_q_partition, p, buf, size);
  if ( err != ESP_OK)
  {
    LOG64_SET(F("EEPROM Q: WRITE ERR["));
    LOG64_SET((uint32_t)err);
    LOG64_SET(F("]"));
    LOG64_SET((uint32_t)p);
    LOG64_SET((uint32_t)size);
    LOG64_NEW_LINE;
  }

}

inline void eeprom_q_erase(uint32_t p, uint32_t size)
{
  esp_err_t err = esp_partition_erase_range(eeprom_q_partition, p, size);
  if ( err != ESP_OK)
  {
    LOG64_SET(F("EEPROM Q: ERASE ERR["));
    LOG64_SET((uint32_t)err);
    LOG64_SET(F("]"));
    LOG64_SET((uint32_t)p);
    LOG64_SET((uint32_t)size);
    LOG64_NEW_LINE;
  }
}

inline bool eeprom_q_is_empty_buf(uint8_t buf[], uint32_t size)
{
  for (uint32_t i = 0; i < size; i++)
  {
    if (buf[i] != 0xFF)
    {
      return false;
    }
  }

  return true;
}

// FFFFFFFF - no valid pos found
inline uint32_t eeprom_q_find_pos_first_last_count_del()
{
  for (uint32_t p = 0; p < EEPROM_Q_TABLE_SIZE; p += 16)
  {
    uint8_t b[16];
    eeprom_q_read(p, (uint8_t *)b, 16);
    if (eeprom_q_is_empty_buf(b, 16))
    {
      if (p == 0)
      {
        return 0xFFFFFFFF;
      }

      return p - 16;
    }
  }

  return EEPROM_Q_TABLE_SIZE - 16;
}
// first, last are relatve positions from EEPROM_Q_DATA_START
inline void eeprom_q_read_first_last_count_del(uint32_t & first, uint32_t & last, uint32_t & count, uint32_t & del)
{
  if ((eeprom_q_first == 0xFFFFFFFF) || (eeprom_q_last == 0xFFFFFFFF) || (eeprom_q_count == 0xFFFFFFFF) || (eeprom_q_del == 0xFFFFFFFF))
  {

    uint32_t p = eeprom_q_find_pos_first_last_count_del();


    //  LOG64_SET(F("EEPROM Q: READ FLCD p["));
    //  LOG64_SET((uint32_t)p);
    //  LOG64_SET(F("]"));
    //  LOG64_NEW_LINE;

    if (p == 0xFFFFFFFF)
    {
      first = 0;
      last = 0;
      count = 0;
      del = 0;
      p = 0;
    }
    else
    {
      uint32_t b[4];
      eeprom_q_read(p, (uint8_t *)b, 16);
      first = b[0];
      last = b[1];
      count = b[2];
      del = b[3];
    }


    eeprom_q_first = first;
    eeprom_q_last = last;
    eeprom_q_count = count;
    eeprom_q_del = del;

    eeprom_q_table_pos = p;

    //  LOG64_SET(F("EEPROM Q: READ FLCD"));
    //  EEPROM_Q_DUMP;
    //  LOG64_NEW_LINE;
  }
  else
  {
    first = eeprom_q_first;
    last = eeprom_q_last;
    count = eeprom_q_count;
    del = eeprom_q_del;
  }
}

// first, last are relatve positions from EEPROM_Q_DATA_START
inline void eeprom_q_write_first_last_count_del(uint32_t first, uint32_t last, uint32_t count, uint32_t del)
{
  uint32_t b[4];
  b[0] = first;
  b[1] = last;
  b[2] = count;
  b[3] = del;

  uint32_t p;
  if (eeprom_q_table_pos == 0xFFFFFFFF)
  {
    p = eeprom_q_find_pos_first_last_count_del();
  }
  else
  {
    p = eeprom_q_table_pos;
  }
  
  if (p == 0xFFFFFFFF)
  {
    p = 0;
  }
  else
  {
    p += 16;
    if (p >= EEPROM_Q_TABLE_SIZE)
    {
      // clean and start from beggining
      eeprom_q_erase(0, EEPROM_Q_TABLE_SIZE);
      p = 0;
    }
  }

  eeprom_q_write(p, (uint8_t *)b, 16);
  eeprom_q_first = first;
  eeprom_q_last = last;
  eeprom_q_count = count;
  eeprom_q_del = del;

  eeprom_q_table_pos = p;
}

/// q inerface implementation
inline void init_q()
{
  const esp_partition_t * eeprom_q_partition_temp = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, EEPROM_Q_PARTIOTION_NAME);
  eeprom_q_partition = (esp_partition_t *)eeprom_q_partition_temp;

  if (eeprom_q_partition == NULL)
  {
    LOG64_SET(F("EEPROM Q: PARITION NOT FOUND"));
    LOG64_NEW_LINE;
  }
  else
  {
    LOG64_SET(F("EEPROM Q:PARTITION FOUND["));
    LOG64_SET(EEPROM_Q_PARTIOTION_NAME);
    LOG64_SET(F("]"));
    LOG64_NEW_LINE;
  }
}

inline void erase_q()
{
  if (eeprom_q_partition == NULL)
  {
    init_q();
  }
  eeprom_q_erase(0, EEPROM_Q_PARTITION_SIZE);
}

inline uint32_t get_q_count()
{
  uint32_t first;
  uint32_t last;
  uint32_t count;
  uint32_t del;

  eeprom_q_read_first_last_count_del(first, last, count, del);

  return count;
}

inline void get_q_first(uint8_t buf[], uint8_t & size)
{
  uint32_t first;
  uint32_t last;
  uint32_t count;
  uint32_t del;

  eeprom_q_read_first_last_count_del(first, last, count, del);

  if (first == last)
  {
    LOG64_SET(F("EEPROM Q: NO ELEMENTS EXTRACT"));
    EEPROM_Q_DUMP;
    LOG64_NEW_LINE;
  }
  else
  {
    uint8_t b[1];
    eeprom_q_read(EEPROM_Q_DATA_START + first, (uint8_t *)b, 1);
    size = b[0];

    if ((first + 1 + b[0]) <= EEPROM_Q_DATA_SIZE)
    {
      // first, last are relatve positions from EEPROM_Q_DATA_START
      eeprom_q_read(EEPROM_Q_DATA_START + first + 1, buf, b[0]);
    }
    else
    {
      uint32_t from_begin = first + 1 + b[0] - EEPROM_Q_DATA_SIZE;
      uint32_t from_end = b[0] - from_begin;
      if (from_end > 0)
      {
        eeprom_q_read(EEPROM_Q_DATA_START + first + 1, buf, from_end);
      }
      if (from_begin > 0)
      {
        eeprom_q_read(EEPROM_Q_DATA_START, &buf[from_end], from_begin);
      }
    }
  }
}

inline void q_first_remove()
{
  uint32_t first;
  uint32_t last;
  uint32_t count;
  uint32_t del;

  eeprom_q_read_first_last_count_del(first, last, count, del);

  if (first == last)
  {
    LOG64_SET(F("EEPROM Q: NO ELEMENTS TO ERASE"));
    EEPROM_Q_DUMP;
    LOG64_NEW_LINE;
  }
  else
  {
    uint8_t b[1];
    eeprom_q_read(first + EEPROM_Q_DATA_START, (uint8_t *)b, 1);
    count -= 1;
    first += b[0] + 1;

    if (first >= EEPROM_Q_DATA_SIZE)
    {
      first -= EEPROM_Q_DATA_SIZE;
    }

    if (del < first)
    {
      for (; first - del > EEPROM_Q_BLOCK_SIZE; del += EEPROM_Q_BLOCK_SIZE)
      {
        eeprom_q_erase(EEPROM_Q_DATA_START + del, EEPROM_Q_BLOCK_SIZE);
      }
    }
    else
    {
      for (; EEPROM_Q_DATA_SIZE >= (del + EEPROM_Q_BLOCK_SIZE); del += EEPROM_Q_BLOCK_SIZE)
      {
        eeprom_q_erase(EEPROM_Q_DATA_START + del, EEPROM_Q_BLOCK_SIZE);
      }

      if (del >= EEPROM_Q_DATA_SIZE)
      {
        del = 0;
      }
    }

    eeprom_q_write_first_last_count_del(first, last, count, del);
  }
}

inline void add_q_last(uint8_t buf[], uint8_t size)
{
  uint32_t first;
  uint32_t last;
  uint32_t count;
  uint32_t del;

  for (;;)
  {

    eeprom_q_read_first_last_count_del(first, last, count, del);

    if (first <= last)
    {
      if ((last + 1 + size) <= EEPROM_Q_DATA_SIZE)
      {

        // most easy case
        uint8_t b[1];
        b[0] = size;
        eeprom_q_write(EEPROM_Q_DATA_START + last , b, 1);
        eeprom_q_write(EEPROM_Q_DATA_START + last + 1, buf, size);
        last += (1 + size);
        if (last >= EEPROM_Q_DATA_SIZE)
        {
          last = 0;
        }

        // all good - element written - exit
        break;
      }
      else
      {
        uint32_t from_begin = (last + 1 + size) - EEPROM_Q_DATA_SIZE;
        uint32_t from_end = size - from_begin;
        if (del >= from_begin)
        {
          // there is space
          uint8_t b[1];
          b[0] = size;
          // there si always space for at least one byte - as if the last is the end of the space it will be moved to 0
          eeprom_q_write( EEPROM_Q_DATA_START + last , b, 1);
          if (from_end > 0)
          {
            eeprom_q_write(EEPROM_Q_DATA_START + last + 1, buf, from_end);
          }
          if (from_begin > 0)
          {
            eeprom_q_write(EEPROM_Q_DATA_START, &buf[from_end], from_begin);
          }
          last = from_begin;
          // as max msg is 256 and the storage is way bigger no need to check how big is last

          // all good - element written - exit
          break;
        }
      }
    }
    else
    {
      if ((last + 1 + size) <= del)
      {
        // there is space
        uint8_t b[1];
        b[0] = size;
        eeprom_q_write( EEPROM_Q_DATA_START + last , b, 1);
        eeprom_q_write( EEPROM_Q_DATA_START + last + 1, buf, size);
        last += (1 + size);
        // as max msg is 255 and the storage is way bigger no need to check how big is last

        // all good - element written - exit
        break;
      }
    }


    // remove first and try again until there is space
    // as msg in max 255 then space will eb always available for at least one msg

    LOG64_SET(F("EEPROM Q: NO SPACE FOR ADD - ERASE count["));
    LOG64_SET((uint32_t)count);
    LOG64_SET(F("]"));
    LOG64_NEW_LINE;

    q_first_remove();
  }

  count += 1;

  eeprom_q_write_first_last_count_del(first, last, count, del);
}

