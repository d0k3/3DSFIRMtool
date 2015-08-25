#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SIZE_CTR_FIRM   0x000F3000
#define OFF_CTR_FIRM0   0x0B130000
#define OFF_CTR_FIRM1   0x0B530000
#define XORPAD_SIZE     (8 * 1024 * 1024)
// #define NODECRYPT

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
    
    // prepare xorpad
    printf("processing... ");
    memset(bufxor, 0x00, XORPAD_SIZE);
    fseek(fp_xor, 0, SEEK_SET);
    fread(bufxor, 1, SIZE_CTR_FIRM, fp_xor);
    fseek(fp_xor, OFF_CTR_FIRM1 - OFF_CTR_FIRM0, SEEK_SET);
    fread(bufxor + OFF_CTR_FIRM1 - OFF_CTR_FIRM0, 1, SIZE_CTR_FIRM, fp_xor);
    
    // apply xorpad
    fread(bufenc, 1, XORPAD_SIZE, fp_in);
    for (size_t i = 0; i < XORPAD_SIZE; i++) bufenc[i] = bufenc[i]^bufxor[i];
    fwrite(bufenc, 1, XORPAD_SIZE, fp_out);
    printf("done!\n\n");
    
    free(bufenc);
    free(bufxor);
    fclose(fp_in);
    fclose(fp_out);
    fclose(fp_xor);
    
    return 1;
}
