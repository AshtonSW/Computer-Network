#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int get_file_size(FILE* fp);
void check_dataword_size(int dataword_size);
void set_generator(const char* gen_str, char* generator, int* generator_len);
void calculate_codeword(char* data, char* generator, int generator_len, int dataword_size, char* code_array);
void binary_to_byte(char* code_array, int arr_idx, char* code_stream);

int main(int argc, char* argv[]) {
	FILE* input_file, * output_file; // CRC로 encode된 파일과 CRC를 제거하고 원래 데이터를 복원한 파일
	int stream_size, file_size, dataword_size, generator_len, code_len, word_cnt;
	char* code_array, * code_stream;
	char generator[10];
	int arr_idx = 0;
		
		
	// usage error
	if (argc != 5) {
		fprintf(stderr, "usage: ./crc_encoder input_file output_file generator dataword_size\n");
		exit(1);
	}

	// Dataword size error
	dataword_size = atoi(argv[4]);
	check_dataword_size(dataword_size);

	// Open the input file
	input_file = fopen(argv[1], "rb");
	if (input_file == NULL) {
		fprintf(stderr, "input file open error.\n");
		return 1;
	}

	// Reopen the input file to get the file size
	input_file = fopen(argv[1], "rb");
	if (input_file == NULL) {
		fprintf(stderr, "input file open error.\n");
		return 1;
	}

	// Open the output file
	output_file = fopen(argv[2], "wb");
	if (output_file == NULL) {
		fprintf(stderr, "output file open error.\n");
		return 1;
	}

	// Get the size of the input file
	file_size = get_file_size(input_file);
	
	// Set the generator polynomial
	set_generator(argv[3], generator, &generator_len);

	// Calculate code_len, word_cnt, and stream_size
	code_len = generator_len - 1 + dataword_size;
	word_cnt = (file_size * 8) / dataword_size;
	stream_size = (code_len * word_cnt + 7) / 8 + 1;

	// Allocate memory for code_stream and code_array
	code_stream = (char*)malloc(sizeof(char) * stream_size);
	code_array = (char*)malloc(sizeof(char) * code_len * word_cnt);

	// Initialize code_stream and code_array
	memset(code_stream, 0, stream_size);
	memset(code_array, 0, code_len * word_cnt);

	// Read the input file
	for (int r = 0; r < file_size; r++) {
		char input, data[8];
		memset(data, 0, 8);

		// Read 1 byte
		fread(&input, 1, 1, input_file);

		// Convert the byte to a binary array
		for (int i = 0; i < 8; i++)
			data[7 - i] = (input >> i) & 1;

		// Split the data into data words
		int num_data_words = (dataword_size == 4) ? 2 : 1;

		for (int i = 0; i < num_data_words; i++) {
			char* codeword;
			char modulus[10];

			// Initialize codeword
			codeword = (char*)malloc(sizeof(char) * code_len);
			memset(codeword, 0, code_len);
			memset(modulus, 0, 10);
			memcpy(codeword, data + (i * dataword_size), dataword_size);

			// Perform polynomial division
			memcpy(modulus, codeword, generator_len);
			int j = 0;
			while (j <= code_len - generator_len) {
				if (modulus[0]) {
					for (int k = 0; k < generator_len; k++) {
						modulus[k] ^= generator[k];
					}
				}

				if (j != code_len - generator_len) {
					memmove(modulus, modulus + 1, generator_len - 1);
					modulus[generator_len - 1] = codeword[j + generator_len];
				}
				j++;
			}

			// Calculate codeword
			memcpy(codeword + dataword_size, modulus + 1, generator_len - 1);
			memcpy(code_array + arr_idx, codeword, code_len);
			arr_idx += code_len;
			free(codeword);
		}
	}

	
	// padding
	code_stream[0] = 8 - (code_len * word_cnt % 8);

	// save encoded code
	binary_to_byte(code_array, code_len * word_cnt, code_stream);

	fwrite(code_stream, stream_size, 1, output_file);
	free(code_stream);
	free(code_array);
	fclose(output_file);
	fclose(input_file);

	return 0;
}


void binary_to_byte(char* code_array, int arr_idx, char* code_stream) {
	int code_idx = 0;
	unsigned char byte_num = 0;
	int j;

	j = 8 - (arr_idx % 8);
	while (j < 8) {
		byte_num += code_array[code_idx++];
		if (j != 7) byte_num <<= 1;
		j++;
	}
	code_stream[1] = byte_num;

	int i = 2;
	while (i < (arr_idx + 7) / 8) {
		byte_num = 0;
		j = 0;
		while (j < 8) {
			byte_num += code_array[code_idx++];
			if (j != 7) byte_num <<= 1;
			j++;
		}
		code_stream[i++] = byte_num;
	}
}

// Get the size of the input file
int get_file_size(FILE* fp) {
	fseek(fp, 0, SEEK_END);
	int file_size = ftell(fp);
	rewind(fp);
	return file_size;
}

// Check if the dataword size is valid
void check_dataword_size(int dataword_size) {
	if (dataword_size != 8 && dataword_size != 4) {
		fprintf(stderr, "dataword size must be 4 or 8.\n");
		exit(1);
	}
}

// Set the generator polynomial and get its length
void set_generator(const char* gen_str, char* generator, int* generator_len) {
	int i = 0;
	memset(generator, 0, 10);
	strcpy(generator, gen_str);
	*generator_len = strlen(generator);

	while (i < *generator_len) {
		generator[i] -= '0';
		i++;
	}
}

// Convert binary array to byte array
void calculate_codeword(char* data, char* generator, int generator_len, int dataword_size, char* code_array) {
	char* codeword;
	char modulus[10];

	codeword = (char*)malloc((dataword_size + generator_len - 1) * sizeof(char));

	// Initial codeword
	memset(codeword, 0, dataword_size + generator_len - 1);
	memcpy(codeword, data, dataword_size);

	// Division
	memcpy(modulus, codeword, generator_len);
	for (int j = 0; j <= dataword_size; j++) {
		if (modulus[0]) {
			for (int k = 0; k < generator_len; k++) {
				modulus[k] ^= generator[k];
			}
		}

		if (j != dataword_size) {
			memmove(modulus, modulus + 1, generator_len - 1);
			modulus[generator_len - 1] = codeword[j + generator_len];
		}
	}

	// Calculate code word
	memcpy(codeword + dataword_size, modulus + 1, generator_len - 1);
	memcpy(code_array, codeword, dataword_size + generator_len - 1);

	free(codeword);
}