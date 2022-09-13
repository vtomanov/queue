
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
// Q-test

///////////////////////////////////////////////////////////////////////////////////
// LOG constants
#define LOG64_ENABLED
#include <Log64.h>

#include "eeprom_queue.h"

void setup()
{
  // Serial Log
  LOG64_INIT();
  //give change to usb serial to connect
#if defined(LOG64_ENABLED)
#define SERIAL_MAX_START_TIMEOUT 5000
  for (uint32_t start = millis(); (!Serial);)
  {
    yield();

    if (((uint32_t)(((uint32_t)millis()) - start)) >= SERIAL_MAX_START_TIMEOUT)
    {
      break;
    }
  }
#else
  delay(2000);
#endif

  init_q();
  erase_q();
}

void loop()
{
  for (uint32_t i = 0; i < 100; i++)
  {


    uint8_t b[250];
    {
      for (uint32_t j = 0; j < 250; j++)
      {
        b[j] = (j + 1);
      }
    }

    uint32_t cnt = rand() % 250;
    uint32_t times = rand() % 10;

    LOG64_SET(F("Q TEST: BEFORE ADD i["));
    LOG64_SET((uint32_t)i);
    LOG64_SET(F("] cnt["));
    LOG64_SET((uint32_t)cnt);
    LOG64_SET(F("] times["));
    LOG64_SET((uint32_t)times);
    LOG64_SET(F("]"));
    LOG64_NEW_LINE;

    {
      for (uint32_t j = 0; j < times; j++)
      {
        add_q_last (b, cnt);
      }
    }
    
    LOG64_SET(F("Q TEST: AFTER ADD count["));
    LOG64_SET(get_q_count());
    LOG64_SET(F("]"));
    LOG64_NEW_LINE;

    {
      for (uint32_t j = 0; j < times; j++)
      {


        {
          uint8_t r[250];
          uint8_t size;

          get_q_first(r, size);

          for (uint32_t k = 0; k < cnt; k++)
          {
            if (b[k] != r[k])
            {
              LOG64_SET(F("Q TEST: PUT/GET ERR"));
              LOG64_NEW_LINE;
            }
          }

        }

        {
          q_first_remove();
        }
      }
    }

    LOG64_SET(F("Q TEST: AFTER READ & DELETE count["));
    LOG64_SET(get_q_count());
    LOG64_SET(F("]"));
    LOG64_NEW_LINE;
  }
}
