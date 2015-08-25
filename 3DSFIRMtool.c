#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SIZE_CTR_FIRM   0x000F3000
#define OFF_CTR_FIRM0   0x0B130000
#define OFF_CTR_FIRM1   0x0B530000

#define FIRM_SIZE       (4 * 1024 * 1024)
#define XORPAD_SIZE     (2 * FIRM_SIZE)
#define UNIT_SIZE       0x1000


void showhelp_exit() {
    printf(" usage: 3DSFIRMtool [-d|-i] [NAND] [FIRM0FIRM1] [XORPAD]\n");
    printf("  -d   Dump FIRM0/FIRM1 from NAND file\n");
    printf("  -i   Inject FIRM0/FIRM1 to NAND file\n");
    exit(0);
}

int main( int argc, char** argv )
{
    FILE* fp_in;
    FILE* fp_out;
    FILE* fp_xor;
    unsigned char* bufenc;
    unsigned char* bufxor;
    int dump;
    
    size_t tsize_f0;
    size_t tsize_f1;
    
    size_t offset;
    size_t size;
    
    printf("\n3DSFIRMtool (C version) by d0k3\n");
    printf("-------------------------------\n\n");
    
    if(argc < 5) showhelp_exit();
    if(strcmp(argv[1], "-d") == 0) dump = 1;
    else if(strcmp(argv[1], "-i") == 0) dump = 0;
    else showhelp_exit();
    
    if(dump) {
        printf("dumping & decrypting %s\n from %s\n using %s\n\n", argv[3], argv[2], argv[4]);
    } else {
        printf("injecting & encrypting %s\n to %s\n using %s\n\n", argv[3], argv[2], argv[4]);
    }
    
    fp_xor = fopen(argv[4], "rb");
    if(fp_xor == NULL) {
        printf("open %s failed!\n\n", argv[4]);
        return 0;
    }
    
    // determine size and type of xorpad
    fseek(fp_xor, 0, SEEK_END);
    size = ftell(fp_xor);
    fseek(fp_xor, 0, SEEK_SET);
    if (size == XORPAD_SIZE) { // FIRM xorpad
        offset = OFF_CTR_FIRM0;
    } else {
        printf("xorpad has bad size!\n\n");
        return 0;
    }        
    
    if(dump) {
        fp_in = fopen(argv[2], "rb");
        if(fp_in == NULL) {
            printf("open %s failed!\n\n", argv[2]);
            return 0;
        }
        fp_out = fopen(argv[3], "wb");
        if(fp_out == NULL) {
            printf("open %s failed!\n\n", argv[3]);
            return 0;
        }
        fseek(fp_in, offset, SEEK_SET);
    } else {
        fp_out = fopen(argv[2], "r+b");
        if(fp_out == NULL) {
            printf("open %s failed!\n\n", argv[2]);
            return 0;
        }
        fp_in = fopen(argv[3], "rb");
        if(fp_in == NULL) {
            printf("open %s failed!\n\n", argv[3]);
            return 0;
        }
        fseek(fp_out, offset, SEEK_SET);
    }
    
    bufenc = (unsigned char*) malloc(XORPAD_SIZE);
    bufxor = (unsigned char*) malloc(XORPAD_SIZE);
    if((bufenc == NULL) || (bufxor == NULL)) {
        printf("out of memory");
        return 0;
    }
    
    // read input data
    fread(bufenc, 1, XORPAD_SIZE, fp_in);
    
    // find firm0firm1 true size
    for (tsize_f0 = FIRM_SIZE; tsize_f0 > 0; tsize_f0--)
        if (bufenc[tsize_f0 - 1] != 0x00) break;
    if (tsize_f0 % UNIT_SIZE) tsize_f0 += UNIT_SIZE - (tsize_f0 % UNIT_SIZE);
    for (tsize_f1 = FIRM_SIZE; tsize_f1 > 0; tsize_f1--)
        if (bufenc[FIRM_SIZE + tsize_f1 - 1] != 0x00) break;
    if (tsize_f1 % UNIT_SIZE) tsize_f1 += UNIT_SIZE - (tsize_f1 % UNIT_SIZE);
    printf("FIRM0 true size: %X\nFIRM1 true size: %X\n", tsize_f0, tsize_f1);
    
    // read and prepare xorpad
    memset(bufxor, 0x00, XORPAD_SIZE);
    fseek(fp_xor, 0, SEEK_SET);
    fread(bufxor, 1, tsize_f0, fp_xor);
    fseek(fp_xor, FIRM_SIZE, SEEK_SET);
    fread(bufxor + FIRM_SIZE, 1, tsize_f1, fp_xor);
    
    // apply xorpad, check for differences
    printf("processing... ");
    if (!dump) { // when injecting: check, then apply
        if (memcmp(bufenc, bufenc + FIRM_SIZE, FIRM_SIZE) != 0)
            printf("\nWARNING: FIRM0 & FIRM1 do not match!\n");
        for (size_t i = 0; i < XORPAD_SIZE; i++)
            bufenc[i] = bufenc[i]^bufxor[i];
    } else { // when dumping: apply, then check
        for (size_t i = 0; i < XORPAD_SIZE; i++)
            bufenc[i] = bufenc[i]^bufxor[i];
        if (memcmp(bufenc, bufenc + FIRM_SIZE, FIRM_SIZE) != 0)
            printf("\nWARNING: FIRM0 & FIRM1 do not match!\n");
    }
    fwrite(bufenc, 1, XORPAD_SIZE, fp_out);
    printf("done!\n\n");
    
    free(bufenc);
    free(bufxor);
    fclose(fp_in);
    fclose(fp_out);
    fclose(fp_xor);
    
    return 1;
}
