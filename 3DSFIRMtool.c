#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define OFF_CTR_FIRM0   0x0B130000
#define OFF_CTR_FIRM1   0x0B530000

#define FIRM_SIZE       (4 * 1024 * 1024)
#define XORPAD_SIZE     (2 * FIRM_SIZE)
#define UNIT_SIZE       0x1000


void showhelp_exit() {
    printf(" usage: 3DSFIRMtool [-d?|-i?] [NAND] [FIRM0|FIRM1] [XORPAD]\n");
    printf("  -d/-d0/-d1   Dump FIRM0/FIRM1 from NAND file\n");
    printf("  -i/-i0/-i1   Inject FIRM0/FIRM1 to NAND file\n");
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
    int firmn;
    
    size_t tsize_firm;
    size_t offset;
    size_t size;
    
    printf("\n3DSFIRMtool (v0.2) by d0k3\n");
    printf("--------------------------\n\n");
    
    if(argc < 5) showhelp_exit();
    if(strlen(argv[1]) > 3) showhelp_exit();
    if(strncmp(argv[1], "-d", 2) == 0) dump = 1;
    else if(strncmp(argv[1], "-i", 2) == 0) dump = 0;
    else showhelp_exit();
    if (argv[1][2] == '0' || !argv[1][2]) firmn = 0;
    else if (argv[1][2] == '1') firmn = 1;
    else showhelp_exit();
    
    if(dump) {
        printf("dumping & decrypting %s\n from %s (firm%i)\n using %s\n\n", argv[3], argv[2], firmn, argv[4]);
    } else {
        printf("injecting & encrypting %s\n to %s (firm%i)\n using %s\n\n", argv[3], argv[2], firmn, argv[4]);
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
    if (size != XORPAD_SIZE) { // FIRM xorpad
        printf("xorpad has bad size!\n\n");
        return 0;
    }

    // adapt for FIRM0 or FIRM1
    if (firmn == 0) { // FIRM0
        offset = OFF_CTR_FIRM0;
    } else { // FIRM1
        offset = OFF_CTR_FIRM1;
        fseek(fp_xor, FIRM_SIZE, SEEK_SET);
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
        // check input firm size
        fseek(fp_in, 0, SEEK_END);
        tsize_firm = ftell(fp_in);
        if (tsize_firm > FIRM_SIZE) {
            printf("%s is too big!\nUse 3DSFAT16tool to write concatenated firms\n\n", argv[2]);
            return 0;
        } else if (tsize_firm < 512) {
            printf("%s is too small!\n\n", argv[2]);
            return 0;
        }
        fseek(fp_in, 0, SEEK_SET);
    }
    
    bufenc = (unsigned char*) malloc(XORPAD_SIZE);
    bufxor = (unsigned char*) malloc(XORPAD_SIZE);
    if((bufenc == NULL) || (bufxor == NULL)) {
        printf("out of memory");
        return 0;
    }
    
    // read input data
    fread(bufenc, 1, FIRM_SIZE, fp_in);
    fread(bufxor, 1, FIRM_SIZE, fp_xor);
    
    // apply xorpad
    printf("processing... ");
    if (!dump) { // when injecting: check, then apply
        if (memcmp(bufenc, "FIRM", 4) != 0) {
            printf("\nnot a proper firm file, won't inject!\n\n");
            return 0;
        }
        for (size_t i = 0; i < tsize_firm; i++)
            bufenc[i] = bufenc[i]^bufxor[i];
    } else { // when dumping: apply, then get size / check
        for (size_t i = 0; i < 512; i++)
            bufenc[i] = bufenc[i]^bufxor[i];
        tsize_firm = 0;
        for (int section = 0; section < 4; section++) {
            size_t f_offs = *(int*) (bufenc + 0x40 + 0x00 + (0x30*section));
            size_t f_size = *(int*) (bufenc + 0x40 + 0x08 + (0x30*section));
            if (!f_size) continue;
            if (f_offs + f_size >= tsize_firm) tsize_firm = f_offs + f_size;
            else {
                tsize_firm = 0;
                break;
            }
        }
        if ((memcmp(bufenc, "FIRM", 4) != 0) || !tsize_firm) {
            printf("\nbad xorpad or corrupt nand.bin, won't dump!\n\n");
            return 0;
        }
        for (size_t i = 512; i < tsize_firm; i++)
            bufenc[i] = bufenc[i]^bufxor[i];
    }
    fwrite(bufenc, 1, tsize_firm, fp_out);
    printf("done!\n\n");
    
    free(bufenc);
    free(bufxor);
    fclose(fp_in);
    fclose(fp_out);
    fclose(fp_xor);
    
    return 1;
}
