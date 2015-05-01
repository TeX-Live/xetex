      if(curchr >= 0xD800 && curchr < 0xDC00)
      {
          UTF16code lower = buffer[curinput.locfield] - 0xDC00;
          incr(curinput.locfield);
          curchr = 0x10000 + (curchr - 0xD800) * 1024 + lower;
      }
