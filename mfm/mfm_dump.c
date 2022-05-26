/* mfm_dump.c
 *
 * Tom Trebisky  5-25-2022
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
// #include <stdint.h>

typedef unsigned char u_char;
typedef unsigned short u_short;
typedef unsigned int u_int;
typedef unsigned long u_int64;

char * tran_path = "callan_raw1";

#define MY_CYL	100
#define MY_HEAD	3

/* ------------------------------ */

void tran_read_all ( char * );
void tran_read_deltas ( char *, int, int, u_short *, int * );
void mfm_scan_marks ( int, int, u_short *, int );
void mfm_decode_deltas ( int, int, u_short *, int );

/* ------------------------------ */

#define TRAN_HEADER_ID_SIZE	8
unsigned char valid_id[] = { 0xee, 0x4d, 0x46, 0x4d, 0x0d, 0x0a, 0x1a, 0x00};

/* There is more in the actual header, but we just read the first part,
 * and seek past things we don't care about to the first track record.
 */
struct tran_header {
    u_char id[TRAN_HEADER_ID_SIZE];
    u_int version;
    u_int fh_size;
    u_int th_size;
    int	cyl;
    int head;
    u_int rate;
};

struct track_header {
    int cyl;
    int head;
    int size;
};

/* On My file anyway, I see a maximum of about  81,500 bytes worth of raw deltas
 *  for a single track.
 */
#define RAW_DELTA_SIZE	85000
#define DELTA_SIZE	85000

u_char raw_deltas[RAW_DELTA_SIZE];
u_short deltas[DELTA_SIZE];
int ndeltas;

/* ------------------------------------------------ */

void
error ( char *msg )
{
    fprintf ( stderr, "Error: %s\n", msg );
    exit ( 1 );
}

int
main ( int argc, char **argv )
{
    // tran_read_all ( tran_path );
    tran_read_deltas ( tran_path, MY_CYL, MY_HEAD, deltas, &ndeltas );
    // mfm_decode_deltas ( MY_CYL, MY_HEAD, deltas, ndeltas );
    mfm_scan_marks ( MY_CYL, MY_HEAD, deltas, ndeltas );

    return 0;
}

/* -------------------------------------------------------- */
/* Transitions file stuff */

void
tran_read_all ( char *path )
{
    struct tran_header hdr;
    struct track_header track_hdr;
    int fd;
    int n;
    off_t pos;

    fd = open ( path, O_RDONLY );

    read ( fd, &hdr, sizeof(hdr) );
    if ( memcmp ( hdr.id, valid_id, sizeof(hdr.id) ) != 0 )
	error ( "Bad file header" );

    // Version: 01020200
    printf ( "Version: %08x\n", hdr.version );
    printf ( "Sample rate: %d\n", hdr.rate );
    printf ( "fh: %d\n", hdr.fh_size );

    pos = hdr.fh_size;

    for ( ;; ) {
	lseek ( fd, pos, SEEK_SET );
	n = read ( fd, &track_hdr, sizeof(track_hdr) );
	if ( n <= 0 )
	    break;
	if ( track_hdr.cyl == -1 &&  track_hdr.head == -1 )
	    break;
	printf ( "Track for %d:%d -- %d bytes\n", track_hdr.cyl, track_hdr.head, track_hdr.size );
	/* Why add 4?  Extra 4 bytes at end of data? */
	pos += track_hdr.size + sizeof(track_hdr) + 4;
    }

    close ( fd );
}

/* My data doesn't have any 2 or 3 byte counts,
 * so this boils down to a plain copy.
 */
void
unpack_deltas ( u_char *raw, int nraw, u_short *deltas, int *count )
{
    int i;
    int ndx;
    u_int value;

    i = 0;
    ndx = 0;
    while ( i < nraw ) {
	if ( raw[i] == 255) {
	    /* XXX how will this fit into 2 bytes ?? */
	    error ( "three byte value" );
            value = raw[i+1] | ((u_int) raw[i+2] << 8) | ((u_int) raw[i+3] << 16);
            i += 4;
         } else if ( raw[i] == 254 ) {
            value = raw[i+1] | ((u_int) raw[i+2] << 8);
            i += 3;
         } else {
            value = raw[i++];
         }

         if ( ndx >= DELTA_SIZE)
	    error ( "Too many deltas" );
         deltas[ndx++] = value;
      }
     printf ( "Unpacked %d deltas\n", ndx );
     *count = ndx;
}

void
tran_read_deltas ( char *path, int cyl, int head, u_short *deltas, int *ndeltas )
{
    struct tran_header hdr;
    struct track_header track_hdr;
    int fd;
    int n;
    off_t pos;

    fd = open ( path, O_RDONLY );

    read ( fd, &hdr, sizeof(hdr) );
    if ( memcmp ( hdr.id, valid_id, sizeof(hdr.id) ) != 0 )
	error ( "Bad file header" );
    // printf ( "Version: %08x\n", hdr.version );
    // printf ( "Sample rate: %d\n", hdr.rate );
    // printf ( "fh: %d\n", hdr.fh_size );

    pos = hdr.fh_size;

    for ( ;; ) {
	lseek ( fd, pos, SEEK_SET );
	n = read ( fd, &track_hdr, sizeof(track_hdr) );
	if ( n <= 0 )
	    break;
	if ( track_hdr.cyl == -1 &&  track_hdr.head == -1 )
	    break;

	pos += track_hdr.size + sizeof(track_hdr) + 4;
	if ( track_hdr.cyl != cyl ||  track_hdr.head != head )
	    continue;

	printf ( "Track for %d:%d -- %d bytes\n", track_hdr.cyl, track_hdr.head, track_hdr.size );
	read ( fd, raw_deltas, track_hdr.size );
	unpack_deltas ( raw_deltas, track_hdr.size, deltas, ndeltas );
	return;
    }

    close ( fd );

    error ( "Did not find requested track" );
}

/* -------------------------------------------------------- */
/* -------------------------------------------------------- */

/* Appropriate for WD1006 family */
#define CONTROLLER_HZ	10000000

#define PRU_HZ		200.0e6		/* 200 Mhz */

// Type II PLL. Here so it will inline. Converted from continuous time
// by bilinear transformation. Coefficients adjusted to work best with
// my data. Could use some more work.
static inline float
filter(float v, float *delay)
{
   float in, out;

   in = v + *delay;
   out = in * 0.034446428576716f + *delay * -0.034124999994713f;
   *delay = in;
   return out;
}

/* Look at the entire track and report all of the 0xA1 marks we find
 * This is useful to determine the number of sectors/track.
 * Each section should have a header mark and a data mark.
 */
void
mfm_scan_marks ( int cyl, int head, u_short *deltas, int ndeltas )
{
    int i;
    int bit_pos;

    /* These are values in units of 200 Mhz clocks.
     * The value will be 20.0 for a 10 Mhz clock.
     */
    float avg_bit_sep_time;
    float nominal_bit_sep_time;

    int track_time = 0;
    float clock_time = 0.0;
    float filter_state = 0;

    // This is the raw MFM data decoded with above
    u_int raw_word = 0;

    int mark = 0;

    /* -------- */

    nominal_bit_sep_time = PRU_HZ / CONTROLLER_HZ;

    avg_bit_sep_time = nominal_bit_sep_time;
    printf ( "Initial bit sep time: %.3f\n", avg_bit_sep_time );

    // printf ( "First deltas: %d %d %d\n", deltas[0], deltas[1], deltas[2] );

    for ( i=1; i< ndeltas; i++ ) {
	track_time += deltas[i];
	clock_time += deltas[i];

	for (bit_pos = 0; clock_time > avg_bit_sep_time / 2;
               clock_time -= avg_bit_sep_time, bit_pos++) ;

	 avg_bit_sep_time = nominal_bit_sep_time + filter(clock_time, &filter_state);

	 /* If the shift is bigger than the word size, then
	  * it pushes the already accumulated bits entirely out
	  * of the word.  This would only happen if something
	  * out of the ordinary was going on.
	  */
         if (bit_pos >= sizeof(raw_word)*8) {
            raw_word = 1;
         } else {
            raw_word = (raw_word << bit_pos) | 1;
         }

	 if ((raw_word & 0xffff) == 0x4489) {
	    mark++;
	    printf ( "Mark (A1) %d at index %d of %d\n", mark, i, ndeltas );
	}
    }

    printf ( "final filtered bit sep time: %.3f\n", avg_bit_sep_time );
}

/* -------------------------------------------------------- */
/* MFM decode stuff */

// Define states for processing the data.
// MARK_ID is looking for the 0xa1 byte
//   before a header
// MARK_DATA is same for the data portion of the sector.
//
// MARK_DATA1 is looking for special Symbolics 3640 mark code.
// MARK_DATA2 is looking for special ROHM PBX mark code.
// DATA_SYNC2 is looking for SUPERBRAIN

// PROCESS_HEADER is processing the header bytes
//  and PROCESS_DATA is processing the data bytes.
// HEADER_SYNC and DATA_SYNC are looking for the one bit to sync to
// in CONTROLLER_XEBEC_104786. Not all decoders use all states.
typedef enum { MARK_ID, MARK_DATA, MARK_DATA1, MARK_DATA2, HEADER_SYNC, HEADER_SYNC2, DATA_SYNC, DATA_SYNC2,
	PROCESS_HEADER, PROCESS_HEADER2, PROCESS_DATA }
STATE_TYPE;

// How many zeros we need to see before we will look for the 0xa1 byte.
// When write turns on and off can cause codes that look like the 0xa1
// so this avoids them. Some drives seem to have small number of
// zeros after sector marked bad in header.
// TODO: look at  v170.raw file and see if number of bits since last
// header should be used to help separate good from false ID.
// also ignoring known write splice location should help.
#define MARK_NUM_ZEROS 2

// Various convenience macros
#define ARRAYSIZE(x) (sizeof(x) / sizeof(x[0]))
#define MAX(x,y) (x > y ? x : y)
#define MIN(x,y) (x < y ? x : y)
#define BIT_MASK(x) (1 << (x))

// This converts the MFM clock and data bits into data bits.
static int code_bits[16] = { 0, 1, 0, 0, 2, 3, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0 };

// This defines all the data in the track. Each operation starts at the
// end of the previous one.
typedef struct trk_l {
      // The count for the field. For TRK_FIll and TRK_FIELD its the number
      // of bytes,
      // for TRK_SUB its the number of times the specificied list should be
      // repeated.
   int count;
      // TRK_FILL fills the specified length of bytes.
   enum {TRK_FILL, TRK_SUB, TRK_FIELD} type;
      // Only used for TRK_FILL.
   u_char value;
      // Pointer to a TRK_L for TRK_SUB or FIELD_L for TRK_FIELD
   void *list;
} TRK_L;

// This contains the polynomial, polynomial length, CRC initial value,
// and maximum span for ECC correction. Use span 0 for no ECC correction.
typedef struct {
   u_int64 init_value;
   u_int64 poly;
   u_int length;
   u_int ecc_max_span;
} CRC_INFO;

// These are the formats we will search through.
struct {
   u_int64 poly;
   int length;
   int ecc_span;
} mfm_all_poly[] = {
  // Length 0 for parity (Symbolics 3640). Doesn't really use length
  // also used for CHECK_NONE
  {0, 0, 0},
  // Length 16 for Northstar header checksum
  {0, 16, 0},
  // Length 32 for Northstar data checksum
  {0, 32, 0},
  // Length 8 for Wang header checksum
  {0, 8, 0},
  // This seemed to have more false corrects than other 32 bit polynomials with
  // more errors than can be corrected. Had false correction at length 5 on disk
  // read so dropped back to 4. My attempt to test showed 7 gives 3-42 false
  // corrections per 100000. Some controllers do 11 bit correct with 32 bit
  // polynomial
  {0x00a00805, 32, 4},
  // Don't move this without fixing the Northstar reference
  {0x1021, 16, 0},
  {0x8005, 16, 0},
  // The rest of the 32 bit polynomials with 8 bit correct get 5-19 false
  // corrects per 100000 when more errors than can be corrected. Reduced due
  // to false correct seen with 0x00a00805
  {0x140a0445, 32, 6},
  {0x140a0445000101ll, 56, 22}, // From WD42C22C datasheet, not tested
  {0x0104c981, 32, 6},
  // The Shugart SA1400 that uses this polynomial says it does 4 bit correct.
  // That seems to have excessive false corrects when more errors that can be
  // corrected so went with 2 bit correct which has 43-141 miscorrects
  // per 100000 for data with more errors than can be corrected.
  {0x24409, 24, 2},
  {0x3e4012, 24, 0}, // WANG 2275. Not a valid ECC code so max correct 0
  {0x88211, 24, 2}, // ROHM_PBX
  // Adaptec bad block on Maxtor XT-2190
  {0x41044185, 32, 6},
  // MVME320 controller
  {0x10210191, 32, 6},
  // Shugart 1610
  {0x10183031, 32, 6},
  // DSD 5217
  {0x00105187, 32, 6},
  // David Junior II DJ_II
  {0x5140c101, 32, 6},
  // Nixdorf
  {0x8222f0804bda23ll, 56, 22}
  // DQ604 Not added to search since more likely to cause false
  // positives that find real matches
  //{0x1, 8, 0}
  // From uPD7261 datasheet. Also has better polynomials so commented out
  //{0x1, 16, 0}
  // From 9410 CRC checker. Not seen on any drive so far
  //{0x4003, 16, 0}
  //{0xa057, 16, 0}
  //{0x0811, 16, 0}
};

struct {
   int length; // -1 indicates valid for all polynomial size
   u_int64 value;
}  mfm_all_init[] = {
   {-1, 0}, {-1, 0xffffffffffffffffll}, {32, 0x2605fb9c}, {32, 0xd4d7ca20},
     {32, 0x409e10aa},
     // 256 byte OMTI
     {32, 0xe2277da8},
     // This is 532 byte sector OMTI. Above are other OMTI. They likely are
     // compensating for something OMTI is doing to the CRC like below
     // TODO: Would be good to find out what. File sun_remarketing/kalok*
     {32, 0x84a36c27},

     // These are for iSBC_215. The final CRC is inverted but special
     // init value will also make it match
     // TODO Add xor to CRC to allow these to be removed
     // header
     {32, 0xed800493},
     // 128 byte sector
     {32, 0xec1f077f},
     // 256 byte sector
     {32, 0xde60050c},
     // 512 byte sector
     {32, 0x03affc1d},
     // 1024 byte sector
     {32, 0xbe87fbf4},
     // This is data area for Altos 586. Unknown why this initial value needed.
     {16, 0xe60c},
     // WANG 2275 with all header bytes in CRC
     {24, 0x223808},
     // This is for DILOG_DQ614, header and data
     {32, 0x58e07342},
     {32, 0xcf2105e0},
     // This is for Convergent AWS on Quantum Q2040 header and data
     {32, 0x920d65c0},
     {32, 0xef26129d},
     {16, 0x8026}, // IBM 3174
     {16, 0x551a} // Altos
  } ;

// CHECK_NONE is used for header formats where some check that is specific
// to the format is used so can't be generalized. If so the check will need
// to be done in the header decode and ext2emu as special cases.
typedef enum {CHECK_CRC, CHECK_CHKSUM, CHECK_PARITY, CHECK_XOR16,
              CHECK_NONE} CHECK_TYPE;

typedef struct {
   char *name;
      // Sector size needs to be smallest value to prevent missing next header
      // for most formats. Some controller formats need the correct value.
   int analyze_sector_size;
      // Rate of MFM clock & data bit cells
   u_int clk_rate_hz;
      // Delay from index pulse to we should start capturing data in
      // nanoseconds. Needed to ensure we start reading with the first
      // physical sector
   u_int start_time_ns;

   int header_start_poly, header_end_poly;
   int data_start_poly, data_end_poly;

   int start_init, end_init;
   enum {CINFO_NONE, CINFO_CHS, CINFO_LBA} analyze_type;

      // Size of headers not including checksum
   int header_bytes, data_header_bytes;
      // These bytes at start of header and data header ignored in CRC calc
   int header_crc_ignore, data_crc_ignore;
   CHECK_TYPE header_check, data_check;

      // These bytes at the end of the data area are included in the CRC
      // but should not be written to the extract file.
   int data_trailer_bytes;
      // 1 if data area is separate from header. 0 if one CRC covers both
   int separate_data;
      // Layout of track.
   TRK_L *track_layout;
      // Sector size to use for converting extract to emulator file
   int write_sector_size;
      // And number of sectors per track
   int write_num_sectors;
      // And first sector number
   int write_first_sector_number;
      // Number of 32 bit words in track MFM data
   int track_words;

      // Non zero of drive has metadata we which to extract.
   int metadata_bytes;
      // Number of extra MFM 32 bit words to copy when moving data around
      // to fix read errors. Formats that need a bunch of zero before
      // a one should use this to copy the zeros.
   int copy_extra;

      // Check information
   CRC_INFO write_header_crc, write_data_crc;
      // Analize is use full search on this format. Model is use
      // the specific data only.
      // TODO: Analize should search model for specific models before
      // doing generic first. We may want to switch this to bit mask if
      // some we will use as specific model then try search with different
      // polynomials.
   enum {CONT_ANALYZE, CONT_MODEL} analyze_search;

      // Minimum number of bits from last good header. Zero if not used
      // Both must be zero or non zero
   int header_min_delta_bits, data_min_delta_bits;
     // Minimum number of bits from index for first header
   int first_header_min_bits;
} CONTROLLER;

/* This is the wd1006 entry, but note the 256 byte sector size XXX */
// I hack and make it 512, we will see how this goes
//    {"WD_1006",              256, 10000000,      0,
static CONTROLLER my_controller = 
    {"WD_1006",              512, 10000000,      0,
         4, ARRAYSIZE(mfm_all_poly), 4, ARRAYSIZE(mfm_all_poly),
         0, ARRAYSIZE(mfm_all_init), CINFO_CHS,
         5, 2, 0, 0, CHECK_CRC, CHECK_CRC,
         0, 1, NULL, 0, 0, 0, 5209,
         0, 0,
         {0,0,0,0},{0,0,0,0}, CONT_ANALYZE,
         0, 0, 0
    };

// The possible states from reading each sector.

// These are ORed into the state
// This is set if information is found such as sector our of the expected
// range. If MODEL controller mostly matches but say has one less sector than
// the disk has it was being selected instead of going on to ANALYZE
#define ANALYZE_WRONG_FORMAT 0x1000
// If this is set the data being CRC'd is zero so the zero CRC
// result is ambiguous since any polynomial will match
#define SECT_AMBIGUOUS_CRC 0x800
// If set treat as error for analyze but otherwise ignore it
#define SECT_ANALYZE_ERROR 0x400
// Set if the sector number is bad when BAD_HEADER is not set.
// Some formats use bad sector numbers to flag bad blocks
#define SECT_BAD_LBA_NUMBER 0x200
#define SECT_BAD_SECTOR_NUMBER 0x100
// This is used to mark sectors that are spare sectors or are marked
// bad and don't contain user data.
// It suppresses counting as errors other errors seen.
#define SECT_SPARE_BAD      0x100
#define SECT_ZERO_HEADER_CRC 0x80
#define SECT_ZERO_DATA_CRC  0x40
#define SECT_HEADER_FOUND   0x20
#define SECT_ECC_RECOVERED  0x10
#define SECT_WRONG_CYL      0x08
// Sector hasn't been written yet
#define SECT_NOT_WRITTEN    0x04
// Only one of these three will be set. BAD_HEADER is initially set
// until we find a good header, then BAD_DATA is set until we find good data
#define SECT_BAD_HEADER     0x02
#define SECT_BAD_DATA       0x01
#define SECT_NO_STATUS      0x00
#define UNRECOVERED_ERROR(x) ((x & (SECT_BAD_HEADER | SECT_BAD_DATA)) && !(x & SECT_SPARE_BAD))

#define MARK_STORED -999

// Number of nanoseconds of each PRU clock
#define CLOCKS_TO_NS 5

// Side of data to skip after header or data area.
#define HEADER_IGNORE_BYTES 10

// For data this is the write splice area where you get corrupted data that
// may look like a sector start byte.
#define DATA_IGNORE_BYTES 10

/* These things would be in the drive_param struct,
 * but we don't have one of those.
 */
#define HEADER_CRC_BITS	16
#define DATA_CRC_BITS	32

/* I am by no means sure about this */
#define MY_SECTOR_SIZE	512
#define MY_SPT	17

/* This is just a place holder for various calles
 * that I don't want to change prototypes for.
 * Stuff in it had better not ever get referenced.
 */
typedef struct {
    int bogus;
} DRIVE_PARAMS;

static DRIVE_PARAMS bogus_params;

typedef u_int SECTOR_DECODE_STATUS;


// The state of a sector
typedef struct {
   // The span of any ECC correction. 0 if no correction
   int ecc_span_corrected_data;
   int ecc_span_corrected_header;
   // The difference between the expected and actual cylinder found
   int cyl_difference;
   // The values from the sector header
   int cyl, head, sector;
   // Non zero if drive is LBA
   int is_lba;
   // The logical block address for LBA drives.
   int lba_addr;
   // A sequential count of sectors starting from 0
   // Logical sector will only be accurate if no header read errors
   // on preceding sectors
   int logical_sector;
   // The sector state
   SECTOR_DECODE_STATUS status;
   SECTOR_DECODE_STATUS last_status;
} SECTOR_STATUS;

SECTOR_DECODE_STATUS wd_process_data (
	STATE_TYPE *, u_char *, int,
        u_int64 , int, int, int *,
        DRIVE_PARAMS *, int *,
        SECTOR_STATUS *, int ,
        SECTOR_DECODE_STATUS );

SECTOR_DECODE_STATUS mfm_process_bytes (
	DRIVE_PARAMS *, u_char *, int , int ,
        STATE_TYPE *, int, int,
        int *, int *,
	SECTOR_STATUS *, SECTOR_DECODE_STATUS );

/* To start with, I am just robbing code from
 * wd_mfm_decode_track_deltas() in David's wd_mfm_decoder.c
 */
void
mfm_decode_deltas ( int cyl, int head, u_short *deltas, int ndeltas )
{
    int i;
    int int_bit_pos;

    /* These are values in units of 200 Mhz clocks.
     * The value will be 20.0 for a 10 Mhz clock.
     */
    float avg_bit_sep_time;
    float nominal_bit_sep_time;

    int track_time = 0;
    float clock_time = 0.0;
    float filter_state = 0;

    // This is the raw MFM data decoded with above
    u_int raw_word = 0;

    // The decoded bits
    u_int decoded_word = 0;

    // Counter to know when we have a bytes worth
    int decoded_bit_cntr = 0;

#define MAX_SECTOR_SIZE 10240 // Data size in bytes
// Max size of raw words for a track. This is big enough to
// hold future growth up to 30 Mbit/sec at 3600 RPM

    // Collect bytes to further process here
    // This is a huge array, enough to hold the entire track.
    // (512*17 = 8704 bytes)
    u_char bytes[MAX_SECTOR_SIZE + 50];

    // how many we have so far
    int byte_cntr = 0;

    // How many we need before passing them to the next routine
    int bytes_needed = 0;
    int header_bytes_needed = 0;

    // Counter to know when to decode the next two bits.
    int raw_bit_cntr = 0;

    // Counter for debugging
    int tot_raw_bit_cntr = 0;

    // Count all the raw bits for emulation file
    // int all_raw_bits_count = 0;

    // Bit count of last of header found
    int header_raw_bit_count = 0;

    // Bit delta between last header and previous header
    int header_raw_bit_delta = 0;

    int zero_count = 0;

    // First address mark time in ns
    int first_addr_mark_ns = 0;

    // Length to perform CRC over
    int bytes_crc_len = 0;
    int header_bytes_crc_len = 0;

    // Where we are in decoding a sector, Start looking for header ID mark
    STATE_TYPE state = MARK_ID;

    // Status of decoding returned
    SECTOR_DECODE_STATUS all_sector_status = SECT_NO_STATUS;

    // Sequential counter for counting sectors
    int sector_index = 0;

    int click = 0;

    /* These would have been arguments to this routine */
    int _seek_difference_static;
    int *seek_difference;
    SECTOR_STATUS sector_status_list[64];

    CONTROLLER *cont = &my_controller;

    /* --- */

    seek_difference = &_seek_difference_static;

    nominal_bit_sep_time = 200.0e6 / my_controller.clk_rate_hz;
    avg_bit_sep_time = nominal_bit_sep_time;
    // printf ( "bit sep time: %d\n", avg_bit_sep_time );

    printf ( "First deltas: %d %d %d\n", deltas[0], deltas[1], deltas[2] );

    for ( i=1; i< ndeltas; i++ ) {
	track_time += deltas[i];
	clock_time += deltas[i];

	for (int_bit_pos = 0; clock_time > avg_bit_sep_time / 2;
               clock_time -= avg_bit_sep_time, int_bit_pos++) ;

	 avg_bit_sep_time = nominal_bit_sep_time + filter(clock_time, &filter_state);

	 /*
	 if (all_raw_bits_count + int_bit_pos >= 32) {
            all_raw_bits_count = mfm_save_raw_word(drive_params,
               all_raw_bits_count, int_bit_pos, raw_word);
         } else {
            all_raw_bits_count += int_bit_pos;
         }
	 */

	 // Shift based on number of bit times then put in the 1 from the
         // delta. If we had a delta greater than the size of raw word we
         // will lose the unprocessed bits in raw_word. This is unlikely
         // to matter since this is invalid MFM data so the disk had a long
         // drop out so many more bits are lost.
         if (int_bit_pos >= sizeof(raw_word)*8) {
            raw_word = 1;
         } else {
            raw_word = (raw_word << int_bit_pos) | 1;
         }

         tot_raw_bit_cntr += int_bit_pos;
         raw_bit_cntr += int_bit_pos;

	 click++;
	 /* I see int_bit_pos of 2, 3, 4 */
	 // printf ( "Raw word %8d: %x (%d,%d)\n", click, raw_word, int_bit_pos, raw_bit_cntr );

         // Are we looking for a mark code?
         if ((state == MARK_ID || state == MARK_DATA)) {

            // These patterns are MFM encoded all zeros or all ones.
            // We are looking for zeros so we assume they are zeros.
	    // We count how many of these before we see the 0xA1
            if (raw_word == 0x55555555 || raw_word == 0xaaaaaaaa) {
               zero_count++;
            } else {
               if (zero_count < MARK_NUM_ZEROS) {
                  zero_count = 0;
               }
            }

	    // This is the 0x891 missing clock MFM sync pattern for 0xA1
            // with all the bits for an 0xa1 so the 16 bit value is 0x4489.
            // This sync is used to mark the header and data fields
            // We want to see enough zeros to ensure we don't get a false
            // match at the boundaries where data is overwritten

	    /* Tom's notes on 4489 - you can decode 4489 to A1
	     * using the code_bits array.  Just note that you take
	     * 4 bits at a time from the source to produce 2 in the result.
	     * So: 4489 -> 2 2 0 1 -> 1010 0001 i.e 0xA1
	     */

	    int use_new_count = 0;  // Use new method
            int good_mark = 0;      // Non zero if proper location to look
            int delta = tot_raw_bit_cntr - header_raw_bit_count;

	    // Only look at headers at the expected location from start of
            // track or from last header.
            if ((raw_word & 0xffff) == 0x4489) {
               // CONTROLLER *cont = &my_controller;
	       printf ( "Bingo with %08x (%d, z=%d)\n", raw_word, i, zero_count );

               if (header_raw_bit_count == 0 && cont->first_header_min_bits != 0) {
                  use_new_count = 1;
                  if (tot_raw_bit_cntr >= cont->first_header_min_bits) {
                     good_mark = 1;
                  }
               }
               if (header_raw_bit_count != 0 && cont->header_min_delta_bits != 0) {
                  use_new_count = 1;
                  if (state == MARK_ID && delta >= cont->header_min_delta_bits) {
                     good_mark = 1;
                  }
                  if (state == MARK_DATA && delta >= cont->data_min_delta_bits) {
                     good_mark = 1;
                  }
               }

               // If not using new bit count method use number of zeros to
               // determine if good mark
               if (!use_new_count && zero_count >= MARK_NUM_ZEROS) {
                  good_mark = 1;
               }
               // printf("Delta %d header %d tot %d use %d good %d\n",
	       //    delta, header_raw_bit_count, tot_raw_bit_cntr, use_new_count, good_mark);
            } /* end of first 4489 case */

	    if ( (raw_word & 0xffff) == 0x4489 && good_mark ) {
		printf ( "Bongo with %08x (%d)\n", raw_word, i );

               if (first_addr_mark_ns == 0) {
                  first_addr_mark_ns = track_time * CLOCKS_TO_NS;
               }
               if (header_raw_bit_count != 0) {
                  header_raw_bit_delta = tot_raw_bit_cntr - header_raw_bit_count;
               }

               header_raw_bit_count = tot_raw_bit_cntr;
               zero_count = 0;

               bytes[0] = 0xa1;
               byte_cntr = 1;

               // header_bytes_crc_len = mfm_controller_info[drive_params->controller].header_bytes +
               //          drive_params->header_crc.length / 8;

               header_bytes_crc_len = my_controller.header_bytes + HEADER_CRC_BITS / 8;

	       header_bytes_needed = header_bytes_crc_len + HEADER_IGNORE_BYTES;

               if (state == MARK_ID) {
                  state = PROCESS_HEADER;
                  // XXX mfm_mark_header_location(all_raw_bits_count, 0, tot_raw_bit_cntr);
                  // Figure out the length of data we should look for
                  bytes_crc_len = header_bytes_crc_len;
                  bytes_needed = header_bytes_needed;
               } else {
                  state = PROCESS_DATA;
                  // XXX mfm_mark_location(all_raw_bits_count, 0, tot_raw_bit_cntr);
                  // Figure out the length of data we should look for

                  // bytes_crc_len = mfm_controller_info[drive_params->controller].data_header_bytes +
                  //       mfm_controller_info[drive_params->controller].data_trailer_bytes +
                  //       drive_params->sector_size +
                  //       drive_params->data_crc.length / 8;
                  bytes_crc_len = my_controller.data_header_bytes +
                        my_controller.data_trailer_bytes +
                        MY_SECTOR_SIZE +
                        DATA_CRC_BITS / 8;

                  bytes_needed = DATA_IGNORE_BYTES + bytes_crc_len;
                  if (bytes_needed >= sizeof(bytes)) {
                     // msg(MSG_FATAL,"Too many bytes needed %d\n", bytes_needed);
		     error ( "Too many bytes needed" );
                     exit(1);
                  }
               }

               // Resync decoding to the mark
               raw_bit_cntr = 0;
               decoded_word = 0;
               decoded_bit_cntr = 0;
            } // end of 4489 case with good mark

	 } else if (state == MARK_DATA1) { // special for SYMBOLICS_3640
	    error ( "MARK_DATA1" );
	 } else if (state == MARK_DATA2) {
	    error ( "MARK_DATA2" );	// special for ROHM_PBX

	 // } else { // PROCESS_DATA
	 } else if (state == PROCESS_DATA || state == PROCESS_HEADER) {

	    // printf ( "Process data/header (%d)\n", i );
	    // error ( "not ready to process data" );

            // Loop while we have enough bits to decode. Stop if state changes
            int entry_state = state;
            while (raw_bit_cntr >= 4 && entry_state == state) {
	       u_int tmp_raw_word;
               // If we have more than 4 only process 4 this time
               raw_bit_cntr -= 4;
               tmp_raw_word = raw_word >> raw_bit_cntr;
               decoded_word = (decoded_word << 2) | code_bits[tmp_raw_word & 0xf];
               decoded_bit_cntr += 2;

               // And if we have a bytes worth store it
               if (decoded_bit_cntr >= 8) {
                  // Do we have enough to further process?

                  if (byte_cntr < bytes_needed) {
                     // If sufficent bits we may have missed a header
                     // 7 is 2 MFM encoded bits and extra for fill fields.
                     // 8 is byte to bits, dup code below
                     if (byte_cntr == header_bytes_needed &&
                           header_raw_bit_delta > header_bytes_needed * 7 * 8) {
                        u_int64 crc;
                        int ecc_span;
                        SECTOR_DECODE_STATUS init_status = 0;

/* I am hacking away, hoping to skip over CRC calculation stuff
 * I just want to dump content.
 */
#define TJT

	#ifdef notyet
                        // Don't perform ECC corrections. They can be
                        // false corrections when not actually sector header.
                        mfm_crc_bytes(drive_params, bytes,
                           header_bytes_crc_len, PROCESS_HEADER, &crc,
                           &ecc_span, &init_status, 0);
	#endif

	#ifdef TJT
			   /* **************** here is where we process bytes.
			    * This bounces through mfm_process_bytes()
			    * in mfm_decoder.c back to wd_process_data()()
			    * in this same file.
			    */ 
                           // all_sector_status |= mfm_process_bytes(drive_params, bytes,
                           all_sector_status |= mfm_process_bytes( &bogus_params, bytes,
                              bytes_crc_len, bytes_needed, &state, cyl, head,
                              &sector_index, seek_difference, sector_status_list, 0);

	#else
                        // We will only get here if processing as data. If
                        // we found a header with good CRC switch to processing
                        // it as a header. Poly != 0 for my testing
                        if (crc == 0 && !(init_status & SECT_AMBIGUOUS_CRC) && drive_params->header_crc.poly != 0) {
			   //printf("Switched header %x\n", init_status);
                           mfm_mark_header_location(MARK_STORED, 0, 0);
                           mfm_mark_end_data(all_raw_bits_count, drive_params);
                           state = PROCESS_HEADER;
                           bytes_crc_len = header_bytes_crc_len;
                           bytes_needed = header_bytes_needed;

                           all_sector_status |= mfm_process_bytes(drive_params, bytes,
                              bytes_crc_len, bytes_needed, &state, cyl, head,
                              &sector_index, seek_difference, sector_status_list, 0);
                        }
	#endif
                     }

		     /* ***** Here is where bytes accumulate */
		     bytes[byte_cntr++] = decoded_word;
		     printf ( "Add byte %8d: %02x (%d)\n", i, decoded_word&0xff, byte_cntr );

                     if (byte_cntr == header_bytes_needed && state == PROCESS_DATA) {
                        // Didn't find header so mark location previously found
                        // as data
	#ifdef notyet
                        mfm_mark_data_location(MARK_STORED, 0, 0);
	#endif
                     }
                  } else {

		     /* Here when "bytes needed" is satisfied */

                     int force_bad = SECT_NO_STATUS;

                     // If data header too far from sector header mark bad.
                     // 7 is 2 MFM encoded bits and extra for fill fields.
                     // 8 is byte to bits, dup code above
                     if (state == PROCESS_DATA &&
                        header_raw_bit_delta > header_bytes_needed * 7 * 8) {
                        force_bad = SECT_BAD_DATA;
			error ( "data too far from header" );
                        //msg(MSG_DEBUG, "Ignored data too far from header %d, %d on cyl %d head %d sector index %d\n",
                        //  header_raw_bit_delta, header_bytes_needed * 7 * 8,
                        //  cyl, head, sector_index);
                     }
	#ifdef notyet
                     mfm_mark_end_data(all_raw_bits_count, drive_params);
	#endif

                     // all_sector_status |= mfm_process_bytes(drive_params, bytes,
                     all_sector_status |= mfm_process_bytes( &bogus_params, bytes,
                        bytes_crc_len, bytes_needed, &state, cyl, head,
                        &sector_index, seek_difference, sector_status_list,
                        force_bad);
                  }
                  decoded_bit_cntr = 0;
               }
            }  // end of decode loop
         } /* end of process data */
	 else {
	    error ( "Unknown state" );
	 }

    }	/* end of deltas loop */
}

/* Process them bytes */

SECTOR_DECODE_STATUS mfm_process_bytes(DRIVE_PARAMS *drive_params,
   u_char bytes[], int bytes_crc_len, int total_bytes,
   STATE_TYPE *state, int cyl, int head,
   int *sector_index, int *seek_difference,
   SECTOR_STATUS sector_status_list[], SECTOR_DECODE_STATUS init_status)
{

   u_int64 crc;
   int ecc_span = 0;
   SECTOR_DECODE_STATUS status = SECT_NO_STATUS;
   char *name;

#ifdef notdef
   if (*state == PROCESS_HEADER) {
      if (msg_get_err_mask() & MSG_DEBUG_DATA) {
         mfm_dump_bytes(bytes, bytes_crc_len, cyl, head, *sector_index,
               MSG_DEBUG_DATA);
      }
      name = "header";
   } else {	/* DATA */
      if (msg_get_err_mask() & MSG_DEBUG_DATA) {
         mfm_dump_bytes(bytes, bytes_crc_len, cyl, head, *sector_index,
               MSG_DEBUG_DATA);
      }
      name = "data";
   }
#endif

   // status = mfm_crc_bytes(drive_params, bytes, bytes_crc_len, *state, &crc,
   //    &ecc_span, &init_status, 1);

   if (crc != 0 || ecc_span != 0) {
      // msg(MSG_DEBUG,"Bad CRC %s cyl %d head %d sector index %d\n",
      //      name, cyl, head, *sector_index);
   }

   // If no error process. Only process with errors if data. Without
   // valid header we don't know what sector we are decoding.
   if (*state != PROCESS_HEADER || crc == 0 || ecc_span != 0) {

         status |= wd_process_data(state, bytes, total_bytes, crc, cyl,
               head, sector_index,
               drive_params, seek_difference, sector_status_list, ecc_span,
               init_status);
    } else {
      // Weren't able to process header to mark invalid if we haven't seen all
      // expected headers. False header detection can occur in the junk at
      // the end of the track.
      // if ( *sector_index < drive_params->num_sectors ) {
      if ( *sector_index < MY_SPT ) {
         status |= SECT_BAD_HEADER;
      }

      // Search for header in case we are out of sync. If we found
      // data next we can't process it anyway.
      *state = MARK_ID;
      printf ( "Out of sync, return to MARK_ID\n" );
   }
   return status;
}

// Perform 3 head bit correction
//
// drive_params: Drive parameters
// head: Head value from header
// exp_head: Expected head
// return: Corrected head value
int
mfm_fix_head(DRIVE_PARAMS *drive_params, int exp_head, int head)
{
#ifdef notdef
   // WD 1003 controllers wrote 3 bit head code so head 8 is written as 0.
   // If requested and head seems correct fix read head value.
   //printf("3 bit %d, head %d exp %d\n",drive_params->head_3bit, head, exp_head);
   if (drive_params->head_3bit && head == (exp_head & 0x7)) {
      return exp_head;
   } else {
      return head;
   }
#endif
      return head;
}


SECTOR_DECODE_STATUS wd_process_data(STATE_TYPE *state, u_char bytes[],
      int total_bytes,
      u_int64 crc, int exp_cyl, int exp_head, int *sector_index,
      DRIVE_PARAMS *drive_params, int *seek_difference,
      SECTOR_STATUS sector_status_list[], int ecc_span,
      SECTOR_DECODE_STATUS init_status)
{
   static int sector_size;
   // Non zero if sector is a bad block, has alternate track assigned,
   // or is an alternate track
   static int bad_block, alt_assigned, is_alternate, alt_assigned_handled;
   static SECTOR_STATUS sector_status;
   // 0 after first sector marked spare/bad found. Only used for Adaptec
   static int first_spare_bad_sector = 1;

    /* ******************************************************* */
    /* ******************************************************* */
    if (*state == PROCESS_HEADER) {
	printf ( "in wd_process_data - header\n" );

	// Clear these since not used by all formats
      alt_assigned = 0;
      alt_assigned_handled = 0;
      is_alternate = 0;
      bad_block = 0;

      memset(&sector_status, 0, sizeof(sector_status));
      sector_status.status |= init_status | SECT_HEADER_FOUND;
      sector_status.ecc_span_corrected_header = ecc_span;
      if (ecc_span != 0) {
         sector_status.status |= SECT_ECC_RECOVERED;
      }

	/* Following is for CONTROLLER_WD_1006 */
	 int sector_size_lookup[4] = {256, 512, 1024, 128};
         int cyl_high_lookup[16] = {0,1,2,3,-1,-1,-1,-1,4,5,6,7,-1,-1,-1,-1};
         int cyl_high;

         cyl_high = cyl_high_lookup[(bytes[1] & 0xf) ^ 0xe];
         sector_status.cyl = 0;
         if (cyl_high != -1) {
            sector_status.cyl = cyl_high << 8;
         }
         sector_status.cyl |= bytes[2];

         sector_status.head = mfm_fix_head(drive_params, exp_head, bytes[3] & 0xf);

	 sector_size = sector_size_lookup[(bytes[3] & 0x60) >> 5];

         bad_block = (bytes[3] & 0x80) >> 7;

         sector_status.sector = bytes[4];

 #ifdef notdef
         // 3B1 with P5.1 stores 4th head bit in bit 5 of sector number field.
         if (drive_params->controller == CONTROLLER_WD_3B1) {
            sector_status.head = sector_status.head | ((sector_status.sector & 0xe0) >> 2);
            sector_status.sector &= 0x1f;
         }
 #endif

         if (cyl_high == -1) {
            //msg(MSG_INFO, "Invalid header id byte %02x on cyl %d,%d head %d,%d sector %d\n",
            //      bytes[1], exp_cyl, sector_status.cyl,
            //      exp_head, sector_status.head, sector_status.sector);
	    printf ( "Invalid header id byte\n" );

            sector_status.status |= SECT_BAD_HEADER;
         }

	 printf ( "Got exp\n" );
	 // msg(MSG_DEBUG,
         //  "Got exp %d,%d cyl %d head %d sector %d,%d size %d bad block %d\n",
         //     exp_cyl, exp_head, sector_status.cyl, sector_status.head,
         //     sector_status.sector, *sector_index, sector_size, bad_block);

      if (bad_block) {
         sector_status.status |= SECT_SPARE_BAD;
         // msg(MSG_INFO,"Bad block set on cyl %d, head %d, sector %d\n",
         //       sector_status.cyl, sector_status.head, sector_status.sector);
	 printf ( "bad block\n" );
      }

      if (is_alternate) {
         // msg(MSG_INFO,"Alternate cylinder set on cyl %d, head %d, sector %d\n",
         //       sector_status.cyl, sector_status.head, sector_status.sector);
	 printf ( "alt cylinder\n" );
      }

#ifdef notdef
      mfm_check_header_values(exp_cyl, exp_head, sector_index, sector_size,
            seek_difference, &sector_status, drive_params, sector_status_list);
#endif


    /* ******************************************************* */
    /* ******************************************************* */
    } else if (*state == PROCESS_DATA) {
	printf ( "in wd_process_data - data\n" );

    // Value and where to look for header mark byte
      int id_byte_expected = 0xf8;
      int id_byte_index = 1;
      int id_byte_mask = 0xff;

      sector_status.status |= init_status;

      if (id_byte_index != -1 &&
            (bytes[id_byte_index] & id_byte_mask) != id_byte_expected &&
            crc == 0) {
         //msg(MSG_INFO,"Invalid data id byte %02x expected %02x on cyl %d head %d sector %d\n",
         //      bytes[id_byte_index], id_byte_expected,
         //      sector_status.cyl, sector_status.head, sector_status.sector);
	 printf ( "Invalid data id byte\n" );
         sector_status.status |= SECT_BAD_DATA;
      }
      if (crc != 0) {
         sector_status.status |= SECT_BAD_DATA;
      }
      if (ecc_span != 0) {
         sector_status.status |= SECT_ECC_RECOVERED;
      }
      sector_status.ecc_span_corrected_data = ecc_span;

      // TODO: If bad sector number the stats such as count of spare/bad
      // sectors is not updated. We need to know the sector # to update
      // our statistics array. This happens with RQDX3
      if (!(sector_status.status & (SECT_BAD_HEADER | SECT_BAD_SECTOR_NUMBER))) {

         // int dheader_bytes = mfm_controller_info[drive_params->controller].data_header_bytes;
         int dheader_bytes = my_controller.data_header_bytes;

#ifdef notdef
         // Bytes[1] is because 0xa1 can't be updated from bytes since
         // won't get encoded as special sync pattern
         if (mfm_write_sector(&bytes[dheader_bytes], drive_params, &sector_status,
               sector_status_list, &bytes[1], total_bytes-1) == -1) {
            sector_status.status |= SECT_BAD_HEADER;
         }
#endif
      }
      if (alt_assigned && !alt_assigned_handled) {
         // msg(MSG_INFO,"Assigned alternate cylinder not corrected on cyl %d, head %d, sector %d\n",
         //       sector_status.cyl, sector_status.head, sector_status.sector);
	 printf ( "Assigned alt cyl not corrected\n" );
      }

      *state = MARK_ID;
      printf ( "state back to MARK_ID\n" );

    /* ******************************************************* */
    /* ******************************************************* */
    } else
	printf ( "in wd_process_data - UNKNOWN !!!\n" );

    return sector_status.status;
}

/* THE END */
