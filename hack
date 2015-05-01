      if(curchr >= 0xD800 && curchr < 0xDC00)
      {
          lower = buffer[curinput.locfield];
          incr(curinput.locfield);
          curchr = (curchr - 0xD800) * 1024 + lower;
      }
