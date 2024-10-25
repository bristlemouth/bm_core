# Utilities

The following document describes some utility functions/macros that are used throughout Bristlemouth:

```{eval-rst}
.. cpp:function:: bm_err_check(err, function)

  This macro is used to check the error state of a tracked `BmErr` variable
  and if there is no error,
  execute a function that returns `BmErr`,
  if executed function returns an error,
  then an error message will be printed describing the error found,
  the line,
  as well as the file the error occurred in.

  :param err: The error value to be evaluated and assigned in the macro
  :param function: Function to be ran and assigned to err if err is BmOK

```

```{eval-rst}
.. cpp:function:: bm_err_check_print(err, function, format, ...)

  This performs the same logic as above
  but adds the ability to print a formatted string after the original error statement described above

  :param err: The error value to be evaluated and assigned in the macro
  :param function: Function to be ran and assigned to err if err is BmOK
  :param format: String with formatting to add to error message
  :param ...: Arguments for format
```

```{eval-rst}
.. cpp:function:: bool is_little_endian(void)

  Determines if the platform architecture is little endian

  :returns: True if little endian, false if big endian
```

```{eval-rst}
.. cpp:function:: void swap_16bit(void *x)

  Swap the endianness of a 16 bit value

  :param x: value to swap endianness of
```

```{eval-rst}
.. cpp:function:: void swap_32bit(void *x)

  Swap the endianness of a 32 bit value

  :param x: value to swap endianness of
```

```{eval-rst}
.. cpp:function:: void swap_64bit(void *x)

  Swap the endianness of a 64 bit value

  :param x: value to swap endianness of
```

```{eval-rst}
.. cpp:function:: size_t bm_strnlen(const char *s, size_t max_length)

  Bristlemouth's implementation of strnlen,
  a non-standard C functions.

  :param s: string to obtain length
  :param max_length: max length of the string

  :returns: size of the string in bytes
```

```{eval-rst}
.. cpp:function:: uint32_t time_remaining(uint32_t start, uint32_t current, uint32_t timeout)

  Calculate the time remaining in a timer,
  this function has no constraints on unit of measurement (seconds, milliseconds, microseconds, etc.),
  but all function parameters must be of that same unit.

  :param start: time the timer was started
  :param current: current time of the timer
  :param timeout: time the timer will be running for
```

```{eval-rst}
.. cpp:function:: uint32_t utc_from_date_time(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second)

  Converts the date from passed in parameters into unix timestamp(UTC).

  :param year: full year (2022, for example)
  :param month: month starting at 1
  :param day: day starting at 1
  :param hour: hour starting at 0 and ending at 23
  :param minute: minute starting at 0 and ending at 59
  :param second: minute starting at 0 and ending at 59

  :returns: UTC seconds since Jan 1st 1970 00:00:00
```

```{eval-rst}
.. cpp:function:: void date_time_from_utc(uint64_t utc_us, UtcDateTime *date_time)

  Convert unix timestamp(UTC) in microseconds to date (YYYYMMDDhhmmss).

  :param utc_us: UTC time in microseconds
  :param date_time: pointer to struct holding the date time values
```
