#include "iniparserClass.h"

/************************************************** DEFINE NEIGHBOURNODE CLASS **********************************************************/

extern void tolowerKeyword(char keywd[]);

// computes sha1 digest for the given keywd
extern int gen_sha1_keywd(unsigned char sha1[], char *keywd); //sha1 array is 20bytes

// computes md5 digest for the given keywd
extern int gen_md5_keywd(unsigned char md5[], char *keywd); //md5 array is 16 byte

// given the sha1 digest, this function gives the sha1 bit to be set in the bit vector
extern unsigned int sha1_keywd_bit(unsigned char sha1[]);

// given the md5 digest, this function gives the md5 bit to be set in the bit vector
extern unsigned int md5_keywd_bit(unsigned char md5[])	;

extern unsigned char hexToByte(unsigned char ch);

/**************************** DEFINE FUNCTION MEMBERS ************************************/

/* DEFAULT CONSTRUCTOR */
bitvec::bitvec()
{
	
	// set every byte as 0 during initialization
	for(int i=0; i<BIT_VECTOR_LEN; i++)
	{
		bitvector[i] = 0;
	}

}

/* DESTRUCTOR */
bitvec::~bitvec()
{
	
	// empty body
}



/**************************** COMMON METHODS ************************************/

/* this function is called twice to set the sha1 bit and md5 bit for the given keywd 
and eventually sets 2 corresponding bits in the bit vector 
*/
int bitvec::setbitInBV(unsigned int bitpos)
{

	// check the whether bitpos is within the range
	if(bitpos < 0 || bitpos > 2*BIT_VECTOR_NBIT_LEN-1)
		return ERROR;

	// byte position in the bitvector for bitpos
	int bytepos = 127 - bitpos/8;
	
	// determine the offset with in the byte by performing mod operation
	int bytemod_pos = bitpos%8;
	
	unsigned char byteToSet = bitvector[bytepos];

	unsigned char newbyte = 0x01 << bytemod_pos;

	if(PRINT_FLAG)
	{
		printf("inside setbitInBV fn.. bitpos is %d, bytepos %d, bytemod_pos %d corresponding hex format is %02x\n", bitpos, bytepos, bytemod_pos, newbyte);
	}
		
	bitvector[bytepos] = (byteToSet | newbyte);
	
	return SUCCESS;


}// end setBV_keywd


// this function determines the sha1 bit and md5 bit for the given keywd and sets 2 corresponding bits in the bit vector
int bitvec::setBV_keywd(char *keywd)
{
	unsigned char sha1gen[SHA_DIGEST_LEN];
	unsigned char md5gen[MD5_DIGEST_LEN];

	// 1. convert the keywd into lower case - case insensitive
	tolowerKeyword(keywd);

	// 2. get the shal value of keywd
	gen_sha1_keywd(sha1gen, keywd);

	// 3. get the sha1 bit position in the 512 bit vector - left half of the bit vector
	unsigned int sha1_bit = sha1_keywd_bit(sha1gen);
	
	// 4. get the md5 value of keywd
	gen_md5_keywd(md5gen, keywd);
	
	// 5. get the md5 bit position in the 512 bit vector - right half of the bit vector
	unsigned int md5_bit = md5_keywd_bit(md5gen);

	// 6. set the corresponding sha1 bit and md5 bit in the bitvector
	if(PRINT_FLAG)
	{
		printf("inside setBV_keywd fn.. keyword considered is %s, sha1_bit %d, md5_bit %d \n", keywd, sha1_bit, md5_bit);
	}

	// setting the left side - sha1, add BIT_VECTOR_NBIT_LEN to sha1bit
	if(setbitInBV(sha1_bit + BIT_VECTOR_NBIT_LEN) == ERROR)
		return ERROR;

	// setting the right side - md5
	if(setbitInBV(md5_bit) == ERROR)
		return ERROR;

	return SUCCESS;


}// end setBV_keywd


// prints the bit vector object in hex string format
void bitvec::printBV_hex() // works on 128 bytes (BIT_VECTOR_LEN)
{
	for(int i=0; i<BIT_VECTOR_LEN; i++)
	{
		if(PRINT_FLAG)
			printf("%02x", bitvector[i]);
	}
	
	if(PRINT_FLAG)
		printf("\n");

	return;

} // end printBV_hex

// stores bit vector in hex string format - to be stored in the file
// bitvec_buf - works on 2*BIT_VECTOR_LEN bytes
void bitvec::storeBV_str(unsigned char bitvec_buf[])
{	
	for(int i=0; i<BIT_VECTOR_LEN; i++)
	{
		sprintf((char *)bitvec_buf, "%s%02x", bitvec_buf, (unsigned char)bitvector[i]);
	}	

	if(PRINT_FLAG)
		printf("in storeBV_str function.. hexstring formed: \n%s \n", bitvec_buf);
	return;
}

// stores bit vector in hex string format - to be stored in the file
// bitvec_buf - works on 2*BIT_VECTOR_LEN bytes
void bitvec::convertFromHex(unsigned char bitvec_buf[])
{	
	
	for(int i=0; i<BIT_VECTOR_LEN; i++)
	{
		unsigned char byte1 = hexToByte(bitvec_buf[2*i]);
		unsigned char byte2 = hexToByte(bitvec_buf[2*i+1]);

		bitvector[i] = (byte1 << 4) + byte2;
	}	
	
	return;

} // convertFromHex


int bitvec_equal(unsigned char b1[], unsigned char b2[])
{
	
	for(int i=0; i<BIT_VECTOR_LEN; i++)
	{
		if(b1[i] != b2[i])
		{
			printf("FALSE \n");
		
			return FALSE;
		}
	}	

	printf("TRUE \n");
	return TRUE;
		
}

void bitvec_and(unsigned char b1[], unsigned char b2[], unsigned char res[])
{

	for(int i=0; i<BIT_VECTOR_LEN; i++)
	{
		res[i] = b1[i] & b2[i];

	}	

}

int bitvec_hit(unsigned char b1[], unsigned char b2[])
{
	bitvec res;
	bitvec_and(b1, b2, res.bitvector);
	
	if(bitvec_equal(b2, res.bitvector) == TRUE)
	{
		return TRUE;
	
	}
	else
	{
		return FALSE;
	
	}

}
