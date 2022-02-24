SECTIONS
{
  . = 0x400000;
  .text :
  {
    *(.text)
    _etext = .;
    __etext = .;
  }
}
