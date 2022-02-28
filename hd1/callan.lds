SECTIONS
{
  . = 0x010000;
  .text :
  {
    *(.text)
    _etext = .;
    __etext = .;
  }
}
