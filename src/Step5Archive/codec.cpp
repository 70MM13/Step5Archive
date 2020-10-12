#include "Step5Archive.h"

using namespace Upp;

/* inspired by SIXPACK.C by Philip G. Gage*/

#define TEXTSEARCH 1000   /* Max strings to search in text file */
#define BINSEARCH   200   /* Max strings to search in binary file */
#define TEXTNEXT     50   /* Max search at next character in text file */
#define BINNEXT      20   /* Max search at next character in binary file */
#define MAXFREQ    2000   /* Max frequency count before table reset */
#define MINCOPY       3   /* Shortest string copy length */
#define MAXCOPY      64   /* Longest string copy length */
#define SHORTRANGE    3   /* Max distance range for shortest length copy */
#define COPYRANGES    6   /* Number of string copy distance bit ranges */
short copybits[COPYRANGES] = {4,6,8,10,12,14};   /* Distance bits */

#define CODESPERRANGE (MAXCOPY - MINCOPY + 1)
int copymin[COPYRANGES], copymax[COPYRANGES];
int maxdistance, maxsize;
int distance, insert = MINCOPY, dictfile = 0, binary = 0;

#define NIL -1                    /* End of linked list marker */
#define HASHSIZE 16384            /* Number of entries in hash table */
#define HASHMASK (HASHSIZE - 1)   /* Mask for hash key wrap */
short *head, *tail;               /* Hash table */
short *succ, *pred;               /* Doubly linked lists */
unsigned char *buffer;            /* Text buffer */

/* Define hash key function using MINCOPY characters of string prefix */
#define getkey(n) ((buffer[n] ^ (buffer[(n+1)%maxsize]<<4) ^ \
                   (buffer[(n+2)%maxsize]<<8)) & HASHMASK)

/* Adaptive Huffman variables */
#define TERMINATE 256             /* EOF code */
#define FIRSTCODE 257             /* First code for copy lengths */
// MAXCHAR is in winnt.h
#define MAX_CHAR (FIRSTCODE+COPYRANGES*CODESPERRANGE-1)
#define SUCCMAX (MAX_CHAR+1)
#define TWICEMAX (2*MAX_CHAR+1)
#define ROOT 1
short left[MAX_CHAR+1], right[MAX_CHAR+1];  /* Huffman tree */
short up[TWICEMAX+1], freq[TWICEMAX+1];

/*** Bit packing routines ***/

int input_bit_count = 0;           /* Input bits buffered */
int input_bit_buffer = 0;          /* Input buffer */
int output_bit_count = 0;          /* Output bits buffered */
int output_bit_buffer = 0;         /* Output buffer */
long bytes_in = 0, bytes_out = 0;  /* File size counters */




Stream *streambuf_src;
Stream *streambuf_dst;
int size_src;
int size_dst;

int codec_error;

int16 head_allocated = 0;
int16 tail_allocated = 0;
int16 succ_allocated = 0;
int16 pred_allocated = 0;
int16 buffer_allocated = 0;

int my_getc(void)
{
	int64 p = streambuf_src->GetPos();
	if (streambuf_src->IsEof()) return EOF;
	if ( (p > size_src) || (p > streambuf_src->GetSize()) || streambuf_src->IsError()) {
		LOG(Format("getc error p=%i size_src=%i GetSize=%i", p, size_src, streambuf_src->GetSize() ));
		codec_error = 0x1158;
		return -1;
	}
	
	int16 c = streambuf_src->Get8();
	//LOG(Format("  my_getc(%i '%c')\x1f", c, c));
	return c;
}

void my_putc(uint8 c)
{
	int64 p = streambuf_dst->GetPos();
	if ( p > size_dst || streambuf_dst->IsError()) {
		codec_error = 0x1158;
		return;
	}
	
	//LOG(Format("  my_putc(%i '%c')\x1f", c, c));
	streambuf_dst->Put ( c );
}

void AllocateMemoryForCompression()
{
	head_allocated = 0;
	tail_allocated = 0;
	succ_allocated = 0;
	pred_allocated = 0;
	buffer_allocated = 0;

	head = new short[HASHSIZE];
	if (head != 0)
	{
		head_allocated = -1;
		tail = new short[HASHSIZE];
		if (tail != 0)
		{
			tail_allocated = -1;
			succ = new short[maxsize];
			if (succ != 0)
			{
				succ_allocated = -1;
				pred = new short[maxsize];
				if (pred != 0)
				{
					pred_allocated = -1;
					buffer = new unsigned char[maxsize];
					if (buffer != 0) {
						memset(buffer, NIL, sizeof(unsigned char)*maxsize); // made Valgrind happy
						buffer_allocated = -1;
					}
					else
						codec_error = 0x1181;
				}
				else
					codec_error = 0x1181;
			}
			else
				codec_error = 0x1181;
		}
		else
			codec_error = 0x1181;
	}
	else
		codec_error = 0x1181;
}

void AllocateMemoryForDecompression()
{
	head_allocated = 0;
	tail_allocated = 0;
	succ_allocated = 0;
	pred_allocated = 0;
	buffer_allocated = 0;
	
	buffer = new unsigned char[maxsize];
	if (buffer != 0)
		buffer_allocated = -1;
	else
		codec_error = 0x1181;
}

void FreeMemory()
{
	if (head_allocated == -1)
		delete[] head;
	if (tail_allocated == -1)
		delete[] tail;
	if (succ_allocated == -1)
		delete[] succ;
	if (pred_allocated == -1)
		delete[] pred;
	if (buffer_allocated == -1)
		delete[] buffer;
}




/* Write one bit to output file */
void output_bit(bool bit)
{
	output_bit_buffer <<= 1;
	if (bit) output_bit_buffer |= 1;
	
	if (++output_bit_count == 8)
	{
		my_putc(output_bit_buffer);
		output_bit_buffer = 0; // not in sixpack.c
		output_bit_count = 0;
		++bytes_out;
	}
}


/* Read a bit from input file */
bool input_bit()
{
	bool bit;
	if (input_bit_count-- == 0)
	{
		input_bit_buffer = my_getc();
		if (input_bit_buffer == EOF) //-1
		{
			// "unexpected end of file" -- handled in my_getc() via codec_error
			input_bit_buffer = 0;
		}
		++bytes_in;
		input_bit_count = 7;
	}
	bit = (input_bit_buffer & 0x80);
	input_bit_buffer <<= 1;
	
	return bit;
}


/* Write multibit code to output file */
void output_code(int16 code, int bits)
{
//	LOG(Format("* PutBits(%i, %i)", code, bits));
	for (int i = 0; i < bits; i++)
	{
		output_bit (code & 1);
		code >>= 1;
	}
}


/* Read multibit code from input file */
int16 input_code(int16 bits)
{
	int16 bit = 1, code = 0;
	for (int16 i = 0; i < bits; i++)
	{
		if (input_bit()) code |= bit;
		bit <<= 1;
	}
//	LOG(Format("* GetBits(%i)=%i", bits, code));
	return code;
}


/* Flush any remaining bits to output file before closing file */
void flush_bits()
{
	if (output_bit_count > 0)
	{
		my_putc(output_bit_buffer << (byte) (8-output_bit_count));
		++bytes_out;
	}
}



/*** Adaptive Huffman frequency compression ***/

/* Data structure based partly on "Application of Splay Trees
   to Data Compression", Communications of the ACM 8/88 */

/* Initialize data for compression or decompression */
void initialize()
{
	int16 i, j;
	distance = 0;
	insert = 3;
	dictfile = 0;
	binary = 0;
	input_bit_count = 0;
	input_bit_buffer= 0;
	output_bit_count = 0;
	output_bit_buffer = 0;
	bytes_in = 0;
	bytes_out = 0;
	
	for (i = 2; i<=TWICEMAX; ++i)
	{
		up[i] = i/2;
		freq[i] = 1;
	}

	for (i = 1; i<=MAX_CHAR; ++i)
	{
		left[i] = 2*i;
		right[i] = 2*i+1;
	}

	j = 0;
	for (i = 0; i<COPYRANGES; ++i)
	{
		copymin[i] = j;
		j += (1 << copybits[i]);
		copymax[i] = j - 1;
	}
	maxdistance = j - 1;
	maxsize = maxdistance + MAXCOPY;
}

/* Update frequency counts from leaf to root */
void update_freq(int16 a, int16 b)
{
	//LOG(Format("----Update2(%i \'%c\', %i \'%c\')", a, a, b, b));
	do {
		freq[up[a]] = freq[a] + freq[b];
		a = up[a];
		if (a != ROOT)
		{
			if (left[up[a]] == a) b = right[up[a]]; //switched from !=
			else b = left[up[a]];
		}
	} while (a != ROOT);
	

  /* Periodically scale frequencies down by half to avoid overflow */
  /* This also provides some local adaption and better compression */
	if (freq[ROOT] == MAXFREQ) //0x7D0; freq+2
	{
		for (a = 1; a <= TWICEMAX; a++) freq[a] >>= 1;
	}
}

/* Update Huffman model for each character code */
void update_model(int16 code)
{
	int16 uua, c, a, ua, b;
	a = code + SUCCMAX;
	++freq [a];
	
	if (up[a] != 1)
	{
		ua = up[a];
		if (left[ua] == a) update_freq(a, right[ua]);
		else update_freq(a, left[ua]);

		do
		{
			uua = up[ua];
			if (left[uua] == ua) b = right[uua];
			else b = left[uua];
			
			/* If high freq lower in tree, swap nodes */
			if (freq[a] > freq[b])
			{
				if (left[uua] == ua) right[uua] = a;
				else left[uua] = a;
					
				if (left[ua] == a) {
					left[ua] = b;
					c = right[ua];
				}
				else {
					right[ua] = b;
					c = left[ua];
				}
				up[b] = ua;
				up[a] = uua;
				update_freq(b, c);
				a = b;
			}
			a = up[a];
			ua = up[a];
		} while (ua != ROOT);
	}
}



/* Compress a character code to output stream */
void compress(int16 code)
{
//	LOG(Format("EncodeChar(%i '%c')%s\x1f", code, code, c>0x100?"                      ***":""));
	
	int16 a, sp=0;
	bool stack[102]; // 50 in sixpack.c
	
	a = code + SUCCMAX; //0x275
	do
	{
		stack[sp++] = (right[up[a]] == a);
		a = up[a];
	} while (a != ROOT);

	do
	{
		output_bit(stack[--sp]);
	} while (sp);

	update_model (code);
}



/* Uncompress a character code from input stream */
int16 uncompress()
{
	int16 c = ROOT;
	do
	{
		if (input_bit()) c = right[c];
		else c = left[c];
	} while (c <= MAX_CHAR); //0x0274 (T + 1) c < T
	
	c -= SUCCMAX;
//	LOG(Format("DecodeChar(%i '%c')%s\x1f", c, c, c>0x100?"                      ***":""));
	update_model(c); //sixpack.c: a-SUCCMAX
	return c;
}



/*** Hash table linked list string search routines ***/

/* Add node to head of list */
void add_node(int16 n)
{
	int16 key;

	key = getkey(n);
	if (head[key] == NIL) {
		tail[key] = n;
		succ[n] = NIL;
	}
	else {
		succ[n] = head[key];
		pred[head[key]] = n;
	}
	head[key] = n;
	pred[n] = NIL;
}

/* Delete node from tail of list */
void delete_node(int16 i)
{
	int16 key;

	key = getkey(i);
	if (head[key] == tail[key]) {
		head[key] = NIL;
	}
	else {
		succ[ pred[tail[key]] ] = NIL;
		tail[key] = pred[ tail[key] ];
	}
}


/* Find longest string matching lookahead buffer string */
int16 match(int16 n, int16 depth)
{
	int16 dist, best, var_8, index, i, len, count, j, var_14, var_16;
	
	best = 0;
	count = 0;

	if (n == maxsize) n = 0;
	int16 key = getkey(n);
	
	index = head[key];
	while (index != NIL)	{
		if (++count > depth) break;
		if ( buffer[(n+best)%maxsize] == buffer[(index + best)%maxsize] )
		{
			len = 0; i = n; j = index;
			while(	((buffer[i] & 0x0FF) == (buffer[j] & 0x0FF))
					&&	(len < MAXCOPY) && (j != n) && (i != insert)  )
			{
				++len;
				if (++i == maxsize) i = 0;
				if (++j == maxsize) j = 0;
			}
			dist = n - index;
			if (dist < 0) dist += maxsize;
			dist -= len;
			if ((dictfile) && (dist > copymax[0])) break;
			if ( len > best && dist <= maxdistance)
				if (len > MINCOPY || dist <= copymax[(binary + 3)]) {
				best = len;
				distance = dist;
			}
		}
		index = succ[index];
	}
	return best;
}



/*** Finite Window compression routines ***/

#define IDLE 0    /* Not processing a copy */
#define COPY 1    /* Currently processing copy */


/* Check first buffer for ordered dictionary file */
/* Better compression using short distance copies */
void dictionary()
{
  int i = 0, j = 0, k, count = 0;

  /* Count matching chars at start of adjacent lines */
  while (++j < MINCOPY+MAXCOPY) {
    if (buffer[j-1] == 10) {
      k = j;
      while (buffer[i++] == buffer[k++]) ++count;
      i = j;
    }
  }
  /* If matching line prefixes > 25% assume dictionary */
  if (count > (MINCOPY+MAXCOPY)/4) dictfile = 1;
}



/* Encode file from input to output */
void encode()
{
	int16 addpos=0, nextlen, state=0, vGetc_result, i, match_len=0, full=0;
	int16 search_start=3;
	
	initialize();
	AllocateMemoryForCompression();

	if ((codec_error & 0xFF) == 0)
	{

		/* Initialize hash table to empty */
		for (i = 0; i < HASHSIZE; ++i) {
			head[i] = NIL;
		}

		/* Compress first few characters using Huffman */
		for (i = 0; i<MINCOPY; i++)
		{
			vGetc_result = my_getc();
			if ((codec_error & 0xFF) != 0) // not in sixpack.c
			{
				goto END;
			}
			if (vGetc_result == EOF)
			{
				goto TERM_END;
			}

			compress(vGetc_result);
			++bytes_in;
			buffer[i] = vGetc_result;
		}
		
		/* Preload next few characters into lookahead buffer */
		for (i = 0; i < MAXCOPY; i++) // 64 is search window?
		{
			vGetc_result = my_getc();
			if ((codec_error & 0xFF) != 0)
			{
				FreeMemory();
				return;
			}
			if (vGetc_result == EOF) break;
			buffer[insert++] = vGetc_result & 0x0FF;
			++bytes_in;
			
			if (vGetc_result > 127) binary = 1; /* Binary file ? */
		}
		dictionary();
		
		while (search_start != insert) {
			/* Check compression to insure really a dictionary file */
			if (dictfile && ((bytes_in % MAXCOPY) == 0))
				if (bytes_in/bytes_out < 2)
					dictfile = 0;     /* Oops, not a dictionary file ! */

			/* Update nodes in hash table lists */
			if (full) delete_node (insert);
			add_node (addpos);
			
			/* If doing copy, process character, else check for new copy */
			if (state == COPY) {
				if (--match_len == 1) state = IDLE;
			}
			else {
				if (binary)
				{
					nextlen = match (search_start+1, BINNEXT);
					match_len = match (search_start, BINSEARCH);
				}
				else {
					nextlen = match (search_start+1, TEXTNEXT);
					match_len = match (search_start, TEXTSEARCH);
				}
				
				/* If long enough and no better match at next char, start copy */
				if (match_len >= MINCOPY && match_len >= nextlen)
				{
					state = COPY;
					
					/* Look up minimum bits to encode distance */
					for (i = 0; i<COPYRANGES; i++)
					{
						if ( distance <= copymax[i] )
						{
							compress( match_len + FIRSTCODE-MINCOPY + (i * CODESPERRANGE) );
							output_code(distance - copymin[i] , copybits[i]);
							break;
						}
					}
				}
				else { /* Else output single literal character */
					compress( buffer[search_start] & 0xFF );
				}
			}
			
			/* Advance buffer pointers */
			if (++search_start == maxsize) search_start = 0;
			if (++addpos == maxsize) addpos = 0;
			
			/* Add next input character to buffer */
			if (vGetc_result != EOF)
			{
				vGetc_result = my_getc();
	
				if ((codec_error & 0xFF) != 0) {
					FreeMemory();
					return;
				}
				
				if (vGetc_result != EOF) {
					buffer[insert++] = (byte) vGetc_result;
					++bytes_in;
				}
				else {
					full = 0;
				}
								
				if (insert == maxsize) {
					insert = 0;
					full = 1;
				}
			}
		}

		goto TERM_END;
	}
	else
	{
		goto END;
	}

TERM_END:
	/* Output EOF code and free memory */
	compress(TERMINATE);
	flush_bits();

END:
	FreeMemory();
}



/* Decode file from input to output */
void decode(void)
{
	int16 c, i, j, k, dist, len, n = 0, index;
	
	initialize();
	AllocateMemoryForDecompression();
	
	while ((c = uncompress()) != TERMINATE) {
		if ((codec_error & 0xFF) != 0) goto END;
		
		if (c < 256)	/* Single literal character ? */
		{
			my_putc(c);
			if ((codec_error & 0xFF) != 0x00) goto END;
			++bytes_out;
			buffer[n++] = c;
			if (n == maxsize) n = 0;
		}
		else			/* Else string copy length/distance codes */
		{
			index = (c - FIRSTCODE) / CODESPERRANGE;
			len = (c - FIRSTCODE + MINCOPY - (index * CODESPERRANGE));
			dist =  input_code(copybits[index])  + len + copymin[index];

			if ((codec_error & 0xFF) != 0) goto END;
			
			j = n;
			k = n - dist;
			if (k < 0) k += maxsize;

			for(i=0; i<len; i++) {
				my_putc(buffer[k] & 0xFF);
				if ((codec_error & 0xFF) != 0) goto END;
				++bytes_out;
				buffer[j++] = buffer[k++];
				
				if (j == maxsize) j = 0;
				if (k == maxsize) k = 0;
			}
			n += len;
			if (n >= maxsize) n -= maxsize;
		}
	}
END:
	FreeMemory();
	//return 0;
}



int EncodeStream(Stream &in, Stream &out, int size_in)
{
	codec_error = 0;
	size_src = size_in + in.GetPos();
	if (size_src < 0) size_src = INT_MAX;
	
	size_dst = INT_MAX;
	
	streambuf_src = &in;
	streambuf_dst = &out;
	encode();
	streambuf_src = 0;
	streambuf_dst = 0;
	return codec_error;
}

int DecodeStream(Stream &in, Stream &out, int size_in, int size_out)
{
	codec_error = 0;
	size_src = size_in + in.GetPos();
	if (size_src < 0) size_src = INT_MAX;
	
	size_dst = size_out + out.GetPos();
	if (size_dst < 0) size_dst = INT_MAX;
	
	streambuf_src = &in;
	streambuf_dst = &out;
	decode();
	streambuf_src = 0;
	streambuf_dst = 0;
	return codec_error;
}
