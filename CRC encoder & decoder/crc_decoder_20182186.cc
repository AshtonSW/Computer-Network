#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void read_input_file(FILE* input_file, int file_size, char* input_stream, int* padding);
void decode_codewords(int code_cnt, int code_len, int generator_len, int dataword_size, char* generator, char* input_stream, int arr_idx, char* data_stream, int* err_cnt);

int main(int argc, char* argv[]) {
    int file_size, generator_len, dataword_size, code_len, code_cnt, word_cnt;
    int err_cnt = 0;
    char generator[10], * input_stream, * data_stream, * output_stream;
    int arr_idx = 0, padding;

    // usage error
    if (argc != 6) {
        fprintf(stderr, "usage: ./crc_decoder input_file output_file result_file generator dataword_size\n");
        exit(1);
    }

    // input file open
    FILE* input_file = fopen(argv[1], "rb");
    if (input_file == NULL) {
        fprintf(stderr, "input file open error.\n");
        return 1;
    }

    // calculate file size
    fseek(input_file, 0, SEEK_END);
    file_size = ftell(input_file);
    fseek(input_file, 0, SEEK_SET);

    // memory allocation for input_stream
    input_stream = (char*)malloc(sizeof(char) * file_size * 8);
    memset(input_stream, 0, file_size * 8);

    // output stream
    output_stream = (char*)malloc(sizeof(char) * file_size);
    memset(output_stream, 0, file_size);

    // output file open
    FILE* output_file = fopen(argv[2], "wb");
    if (output_file == NULL) {
        fprintf(stderr, "output file open error.\n");
        return 1;
    }

    // result file open
    FILE* re_file = fopen(argv[3], "w");
    if (re_file == NULL) {
        fprintf(stderr, "result file open error.\n");
        return 1;
    }

    // dataword size error
    dataword_size = atoi(argv[5]);
    if (dataword_size != 4 && dataword_size != 8) {
        fprintf(stderr, "dataword size must be 4 or 8.\n");
        return 1;
    }

    // generator setting
    memset(generator, 0, sizeof(generator));
    strcpy(generator, argv[4]);
    generator_len = strlen(generator);
    for (int i = 0; i < generator_len; i++) {
        generator[i] -= '0';
    }

    // codeword setting
    code_len = dataword_size + generator_len - 1;

    // read input file
    read_input_file(input_file, file_size, input_stream, &padding);
    fclose(input_file); // close input file after reading
    arr_idx = padding + 8;
    code_cnt = (file_size * 8 - arr_idx) / code_len;

    // data stream
    if (dataword_size == 4)
        word_cnt = code_cnt / 2;
    else
        word_cnt = code_cnt;

    data_stream = (char*)malloc(sizeof(char) * code_cnt * dataword_size);
    memset(data_stream, 0, code_cnt * dataword_size);

    // decode
    decode_codewords(code_cnt, code_len, generator_len, dataword_size, generator, input_stream, arr_idx, data_stream, &err_cnt);

    // convert data stream to output stream
    int output_idx = 0;
    for (int i = 0; i < word_cnt * 8; i++) {
        output_stream[output_idx] += data_stream[i];
        if (i % 8 == 7) {
            output_idx++;
        }
        else {
            output_stream[output_idx] <<= 1;
        }
    }

    // file write
    fprintf(re_file, "%d %d\n", code_cnt, err_cnt);
    fwrite(output_stream, output_idx, 1, output_file);

    // file close
    fclose(output_file);
    fclose(re_file);

    // free allocated memory
    free(data_stream);
    free(input_stream);
    free(output_stream);

}

void read_input_file(FILE* input_file, int file_size, char* input_stream, int* padding) {
    for (int i = 0; i < file_size; i++) {
        char input;
        fread(&input, 1, 1, input_file);
        if (i == 0) *padding = input;
        for (int j = 0; j < 8; j++) {
            input_stream[i * 8 + j] = (input >> (7 - j)) & 1;
        }
    }
}

void decode_codewords(int code_cnt, int code_len, int generator_len, int dataword_size, char* generator, char* input_stream, int arr_idx, char* data_stream, int* err_cnt) {
    char modulus[10], codeword[code_len];
    int data_idx = 0;
    for (int r = 0; r < code_cnt; r++) {
        memset(codeword, 0, code_len);
        memcpy(codeword, input_stream + arr_idx, code_len);
        arr_idx += code_len;

        memset(modulus, 0, 10);

        // division
        memcpy(modulus, codeword, generator_len);
        for (int j = 0; j <= code_len - generator_len; j++) {
            if (modulus[0]) {
                for (int k = 0; k < generator_len; k++) {
                    modulus[k] ^= generator[k];
                }
            }
            if (j != code_len - generator_len) {
                memcpy(modulus, modulus + 1, generator_len - 1);
                modulus[generator_len - 1] = codeword[j + generator_len];
            }
        }

        // error detect
        memcpy(modulus, modulus + 1, generator_len - 1);
        int error_detected = 0;
        for (int j = 0; j < generator_len - 1; j++) {
            if (modulus[j] != 0) {
                error_detected = 1;
                break;
            }
        }
        if (error_detected) (*err_cnt)++;

        // data save
        for (int j = 0; j < dataword_size; j++) {
            data_stream[data_idx + j] = error_detected ? 0 : codeword[j];
        }
        data_idx += dataword_size;
    }
}