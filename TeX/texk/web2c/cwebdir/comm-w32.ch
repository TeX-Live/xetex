This is the change file for CWEB's COMMON under Win32
(Contributed by Fabrice Popineau, February 2002 <Fabrice.Popineau@supelec.fr>)

@x section 70
  if (found_change<=0) strcpy(change_file_name,"/dev/null");
@y
  if (found_change<=0) strcpy(change_file_name,"NUL");
@z
