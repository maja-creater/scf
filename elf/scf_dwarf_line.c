#include"scf_dwarf_def.h"
#include"scf_leb128.h"
#include"scf_native.h"
#include"scf_elf.h"

scf_dwarf_line_machine_t* scf_dwarf_line_machine_alloc()
{
	scf_dwarf_line_machine_t* lm;

	lm = calloc(1, sizeof(scf_dwarf_line_machine_t));
	if (!lm)
		return NULL;

	lm->prologue = calloc(1, sizeof(scf_dwarf_line_prologue_t));
	if (!lm->prologue) {
		free(lm);
		return NULL;
	}
	return lm;
}

void scf_dwarf_line_filename_free(scf_dwarf_line_filename_t* f)
{
	if (f) {
		if (f->name)
			free(f->name);
		free(f);
	}
}

void scf_dwarf_line_machine_free(scf_dwarf_line_machine_t* lm)
{
	if (lm) {
		if (lm->prologue) {

			if (lm->prologue->standard_opcode_lengths)
				free(lm->prologue->standard_opcode_lengths);

			if (lm->prologue->file_names) {
				scf_vector_clear(lm->prologue->file_names, ( void (*)(void*) )scf_dwarf_line_filename_free);
				scf_vector_free(lm->prologue->file_names);
			}

			free(lm->prologue);
		}

		free(lm);
	}
}

static scf_dwarf_ubyte_t standard_opcode_lengths[13] = {
	0, 1, 1, 1, 1,
	0, 0, 0, 1, 0,
	0, 1, 0
};

int scf_dwarf_line_machine_fill(scf_dwarf_line_machine_t* lm, scf_vector_t* file_names)
{
	if (!lm)
		return -EINVAL;

	lm->address = 0;
	lm->file    = 1;
	lm->line    = 1;
	lm->column  = 0;
	lm->is_stmt = 1;
	lm->basic_block  = 0;
	lm->end_sequence = 0;

	lm->prologue->total_length = 0;
	lm->prologue->version      = 2;
	lm->prologue->prologue_length = 0;
	lm->prologue->minimum_instruction_length = 1;
	lm->prologue->default_is_stmt = 1;
	lm->prologue->line_base       = -5;
	lm->prologue->line_range      = 14;
	lm->prologue->opcode_base     = 13;

	lm->prologue->standard_opcode_lengths = calloc(lm->prologue->opcode_base, sizeof(scf_dwarf_ubyte_t));
	if (!lm->prologue->standard_opcode_lengths)
		return -ENOMEM;

	memcpy(lm->prologue->standard_opcode_lengths, standard_opcode_lengths, 13 * sizeof(scf_dwarf_ubyte_t));

	lm->prologue->include_directories = NULL;

	lm->prologue->file_names = scf_vector_alloc();
	if (!lm->prologue->file_names)
		return -ENOMEM;

	scf_dwarf_line_filename_t* f;
	int i;

	for (i = 0; i  < file_names->size; i++) {

		scf_string_t* fname = file_names->data[i];

		f = calloc(1, sizeof(scf_dwarf_line_filename_t));
		if (!f)
			return -ENOMEM;

		f->name = malloc(fname->len + 1);
		if (!f->name) {
			free(f);
			return -ENOMEM;
		}

		memcpy(f->name, fname->data, fname->len);
		f->name[fname->len] = '\0';

		if (scf_vector_add(lm->prologue->file_names, f) < 0) {
			free(f->name);
			free(f);
			return -ENOMEM;
		}
	}

	return 0;
}

void scf_dwarf_line_machine_print(scf_dwarf_line_machine_t* lm)
{
	if (lm) {
		printf("lm->address: %#lx\n", lm->address);
		printf("lm->file:    %u\n",   lm->file);
		printf("lm->line:    %u\n",   lm->line);
		printf("lm->column:  %u\n",   lm->column);
		printf("lm->is_stmt: %u\n",   lm->is_stmt);
		printf("lm->basic_block:  %u\n", lm->basic_block);
		printf("lm->end_sequence: %u\n", lm->end_sequence);

		printf("prologue->total_length:    %u\n", lm->prologue->total_length);
		printf("prologue->version:         %u\n", lm->prologue->version);
		printf("prologue->prologue_length: %u\n", lm->prologue->prologue_length);
		printf("prologue->minimum_instruction_length: %u\n", lm->prologue->minimum_instruction_length);
		printf("prologue->default_is_stmt: %u\n", lm->prologue->default_is_stmt);

		printf("prologue->line_base:   %d\n", lm->prologue->line_base);
		printf("prologue->line_range:  %u\n", lm->prologue->line_range);
		printf("prologue->opcode_base: %u\n", lm->prologue->opcode_base);

		printf("prologue->standard_opcode_lengths: %p\n", lm->prologue->standard_opcode_lengths);
		int i;
		for (i = 0; i < lm->prologue->opcode_base; i++) {
			printf("i: %d, len: %u\n", i, lm->prologue->standard_opcode_lengths[i]);
		}

		printf("prologue->include_directories: %p\n", lm->prologue->include_directories);

		scf_dwarf_line_filename_t* f;

		for (i = 0; i < lm->prologue->file_names->size; i++) {
			f  =        lm->prologue->file_names->data[i];

			printf("i: %d, name:          %s\n", i, f->name);
			printf("i: %d, dir_index:     %u\n", i, f->dir_index);
			printf("i: %d, time_modified: %u\n", i, f->time_modified);
			printf("i: %d, length:        %u\n", i, f->length);
		}
	}
}

static inline void _decode_special_opcode(scf_dwarf_line_prologue_t* prologue, scf_dwarf_ubyte_t opcode, scf_dwarf_uword_t* paddress_advance, scf_dwarf_sword_t* pline_advance)
{
	opcode -= prologue->opcode_base;
	*paddress_advance = opcode / prologue->line_range;
	*pline_advance    = opcode % prologue->line_range + (scf_dwarf_sword_t)prologue->line_base;
}

int scf_dwarf_line_decode(scf_dwarf_line_machine_t* lm, scf_vector_t* line_results, const char*   debug_line, size_t debug_line_size)
{
	if (!lm || !line_results || !debug_line || debug_line_size < sizeof(scf_dwarf_uword_t)) {
		scf_loge("\n");
		return -EINVAL;
	}

	lm->prologue->total_length = *(scf_dwarf_uword_t*)debug_line;

	if (sizeof(scf_dwarf_uword_t) + lm->prologue->total_length != debug_line_size) {
		scf_loge("\n");
		return -EINVAL;
	}

	size_t i = sizeof(scf_dwarf_uword_t);

	lm->prologue->version = *(scf_dwarf_uhalf_t*)(debug_line + i);
	i += sizeof(scf_dwarf_uhalf_t);

	lm->prologue->prologue_length = *(scf_dwarf_uword_t*)(debug_line + i);
	i += sizeof(scf_dwarf_uword_t);

	lm->prologue->minimum_instruction_length = *(scf_dwarf_ubyte_t*)(debug_line + i);
	i += sizeof(scf_dwarf_ubyte_t);

	lm->prologue->default_is_stmt = *(scf_dwarf_ubyte_t*)(debug_line + i);
	i += sizeof(scf_dwarf_ubyte_t);

	lm->prologue->line_base   = *(scf_dwarf_sbyte_t*)(debug_line + i);
	i += sizeof(scf_dwarf_sbyte_t);

	lm->prologue->line_range  = *(scf_dwarf_ubyte_t*)(debug_line + i);
	i += sizeof(scf_dwarf_ubyte_t);

	lm->prologue->opcode_base = *(scf_dwarf_ubyte_t*)(debug_line + i);
	i += sizeof(scf_dwarf_ubyte_t);


	size_t n = lm->prologue->opcode_base - 1;
	if (lm->prologue->standard_opcode_lengths)
		free(lm->prologue->standard_opcode_lengths);

	lm->prologue->standard_opcode_lengths = malloc(n * sizeof(scf_dwarf_ubyte_t));
	if (!lm->prologue->standard_opcode_lengths)
		return -ENOMEM;

	n = 0;
	while (i < debug_line_size
			&& n < lm->prologue->opcode_base - 1) {
		size_t   len = 0;
		uint32_t val = scf_leb128_decode_uint32(debug_line + i, &len);
		assert(len > 0);

		lm->prologue->standard_opcode_lengths[n++] = val;
		i += len;
	}

	if (n < lm->prologue->opcode_base - 1) {
		scf_loge("\n");
		return -1;
	}

	assert(i < debug_line_size);
	if ('\0' == debug_line[i]) {
		lm->prologue->include_directories = NULL;
		i++;
	} else {
		assert(0);
	}

	if (!lm->prologue->file_names) {
		lm->prologue->file_names = scf_vector_alloc();
		if (!lm->prologue->file_names)
			return -ENOMEM;
	} else {
		scf_vector_clear(lm->prologue->file_names, (void (*)(void*))free);
	}

	while (i < debug_line_size) {
		if ('\0' == debug_line[i])
			break;

		size_t j = i++;
		while (i < debug_line_size && '\0' != debug_line[i])
			i++;

		assert('\0' == debug_line[i]);
		assert(i < debug_line_size);

		scf_dwarf_line_filename_t* filename = calloc(1, sizeof(scf_dwarf_line_filename_t));
		if (!filename)
			return -ENOMEM;

		filename->name = malloc(i - j + 1);
		if (!filename->name) {
			free(filename);
			return -ENOMEM;
		}
		memcpy(filename->name, debug_line + j, i - j);
		filename->name[i - j] = '\0';

		i++;
		assert(i < debug_line_size);

		size_t   len = 0;
		uint32_t val = scf_leb128_decode_uint32(debug_line + i, &len);
		assert(len > 0);

		filename->dir_index = val;
		i += len;
		assert(i < debug_line_size);


		len = 0;
		val = scf_leb128_decode_uint32(debug_line + i, &len);
		assert(len > 0);

		filename->time_modified = val;
		i += len;
		assert(i < debug_line_size);


		len = 0;
		val = scf_leb128_decode_uint32(debug_line + i, &len);
		assert(len > 0);

		filename->length = val;
		i += len;
		assert(i < debug_line_size);

		int ret = scf_vector_add(lm->prologue->file_names, filename);
		if (ret < 0) {
			scf_loge("\n");
			return ret;
		}
	}
	assert(i < debug_line_size);
	assert('\0' == debug_line[i]);
	i++;

	lm->address = 0;
	lm->file    = 1;
	lm->line    = 1;
	lm->column  = 0;
	lm->is_stmt = lm->prologue->default_is_stmt;
	lm->basic_block  = 0;
	lm->end_sequence = 0;

	while (i < debug_line_size) {

		scf_dwarf_line_result_t* result = NULL;
		int ret;

		scf_dwarf_ubyte_t opcode;
		scf_dwarf_uword_t address_advance;
		scf_dwarf_sword_t line_advance;
		scf_dwarf_uhalf_t uhalf;

		opcode = debug_line[i++];

		if (opcode >= lm->prologue->opcode_base) {

			_decode_special_opcode(lm->prologue, opcode, &address_advance, &line_advance);
			lm->address += address_advance * lm->prologue->minimum_instruction_length;
			lm->line    += line_advance;

			result = calloc(1, sizeof(scf_dwarf_line_result_t));
			if (!result)
				return -ENOMEM;

			result->address = lm->address;
			result->line    = lm->line;
			result->column  = lm->column;
			result->is_stmt = lm->is_stmt;
			result->basic_block  = lm->basic_block;
			result->end_sequence = lm->end_sequence;

			ret = scf_vector_add(line_results, result);
			if (ret < 0)
				return ret;

			lm->basic_block = 0;
			result = NULL;

			opcode -= lm->prologue->opcode_base;

		} else if (opcode > 0) {
			size_t len;

			switch (opcode) {
				case DW_LNS_copy:
					result = calloc(1, sizeof(scf_dwarf_line_result_t));
					if (!result)
						return -ENOMEM;

					result->address = lm->address;
					result->line    = lm->line;
					result->column  = lm->column;
					result->is_stmt = lm->is_stmt;
					result->basic_block  = lm->basic_block;
					result->end_sequence = lm->end_sequence;

					ret = scf_vector_add(line_results, result);
					if (ret < 0)
						return ret;

					scf_logd("opcode: %u, address: %#lx, line: %u\n", opcode, lm->address, lm->line);

					lm->basic_block = 0;
					result = NULL;
					break;

				case DW_LNS_advance_pc:
					len = 0;
					address_advance  = scf_leb128_decode_uint32(debug_line + i, &len);
					address_advance *= lm->prologue->minimum_instruction_length;
					lm->address     += address_advance;

					scf_logd("opcode: %u, address: %#lx, line: %u, address_advance: %u\n",
							opcode, lm->address, lm->line, address_advance);

					assert(len > 0);
					i += len;
					break;

				case DW_LNS_advance_line:
					len = 0;
					line_advance  = scf_leb128_decode_int32(debug_line + i, &len);
					lm->line     += line_advance;

					assert(len > 0);
					i += len;
					break;

				case DW_LNS_set_file:
					len = 0;
					lm->file = scf_leb128_decode_uint32(debug_line + i, &len);

					assert(len > 0);
					i += len;
					break;

				case DW_LNS_set_column:
					len = 0;
					lm->column = scf_leb128_decode_uint32(debug_line + i, &len);

					assert(len > 0);
					i += len;
					break;

				case DW_LNS_negate_stmt:
					lm->is_stmt = !lm->is_stmt;
					break;

				case DW_LNS_set_basic_block:
					lm->basic_block = 1;
					break;

				case DW_LNS_const_add_pc:
					_decode_special_opcode(lm->prologue, 255, &address_advance, &line_advance);
					lm->address += address_advance;
					break;

				case DW_LNS_fixed_advance_pc:
					assert(debug_line_size - i >= 2);
					assert(2 == sizeof(scf_dwarf_uhalf_t));

					uhalf   = debug_line[i + 1];
					uhalf <<= 8;
					uhalf  |= debug_line[i];

					lm->address += uhalf;
					i += 2;
					break;
				default:
					scf_loge("\n");
					return -1;
					break;
			}
		} else {
			assert(0 == opcode);

			size_t len = 0;
			size_t num = scf_leb128_decode_uint32(debug_line + i, &len);
			assert(len > 0);
			assert(num > 0);
			i += len;

			assert(i + num <= debug_line_size);

			opcode = debug_line[i];
			if (DW_LNE_end_sequence == opcode) {

				result = calloc(1, sizeof(scf_dwarf_line_result_t));
				if (!result)
					return -ENOMEM;

				lm->end_sequence = 1;

				result->address  = lm->address;
				result->line     = lm->line;
				result->column   = lm->column;
				result->is_stmt  = lm->is_stmt;
				result->basic_block  = lm->basic_block;
				result->end_sequence = lm->end_sequence;

				ret = scf_vector_add(line_results, result);
				if (ret < 0)
					return ret;

				scf_logd("opcode: %u, address: %#lx, line: %u\n", opcode, lm->address, lm->line);

				lm->address = 0;
				lm->file    = 1;
				lm->line    = 1;
				lm->column  = 0;
				lm->is_stmt = lm->prologue->default_is_stmt;
				lm->basic_block  = 0;
				lm->end_sequence = 0;

				i += num;

			} else if (DW_LNE_set_address == opcode) {
				assert(1 + sizeof(uintptr_t) == num);

				uintptr_t address = 0;
				int j;
				for (j = sizeof(uintptr_t) - 1; j >= 0; j--) {
					address <<= 8;
					address  |= debug_line[i + 1 + j];
				}

				lm->address = address;
				i += num;

			} else if (DW_LNE_define_file == opcode) {
				int j = i + 1;
				while (j < debug_line_size && '\0' != debug_line[j])
					j++;
				assert('\0' == debug_line[j]);

				scf_loge("file: %s\n", debug_line + i + 1);

				j++;
				len = 0;
				uint32_t dir_index = scf_leb128_decode_uint32(debug_line + j, &len);
				assert(len > 0);
				j += len;

				len = 0;
				uint32_t time_modified = scf_leb128_decode_uint32(debug_line + j, &len);
				assert(len > 0);
				j += len;

				len = 0;
				uint32_t file_length = scf_leb128_decode_uint32(debug_line + j, &len);
				assert(len > 0);
				j += len;

				scf_loge("dir_index:     %u\n", dir_index);
				scf_loge("time_modified: %u\n", time_modified);
				scf_loge("file_length:   %u\n", file_length);

				i += num;
				assert(j == i);
			} else {
				scf_loge("\n");
				return -1;
			}
		}
	}

	return 0;
}

int scf_dwarf_line_encode(scf_dwarf_debug_t* debug, scf_dwarf_line_machine_t* lm, scf_vector_t* line_results, scf_string_t* debug_line)
{
	if (!debug || !lm || !line_results || !debug_line)
		return -EINVAL;

#define DWARF_DEBUG_LINE_FILL(data) \
	do { \
		int ret = scf_string_cat_cstr_len(debug_line, (char*)&(data), sizeof(data)); \
		if (ret < 0) { \
			scf_loge("\n"); \
			return ret; \
		} \
	} while (0)

#define DWARF_DEBUG_LINE_FILL2(buf, len) \
	do { \
		int ret = scf_string_cat_cstr_len(debug_line, (char*)buf, len); \
		if (ret < 0) { \
			scf_loge("\n"); \
			return ret; \
		} \
	} while (0)

	size_t origin_len = debug_line->len;

	DWARF_DEBUG_LINE_FILL(lm->prologue->total_length);
	DWARF_DEBUG_LINE_FILL(lm->prologue->version);

	size_t prologue_pos = debug_line->len;

	DWARF_DEBUG_LINE_FILL(lm->prologue->prologue_length);

	DWARF_DEBUG_LINE_FILL(lm->prologue->minimum_instruction_length);
	DWARF_DEBUG_LINE_FILL(lm->prologue->default_is_stmt);
	DWARF_DEBUG_LINE_FILL(lm->prologue->line_base);
	DWARF_DEBUG_LINE_FILL(lm->prologue->line_range);
	DWARF_DEBUG_LINE_FILL(lm->prologue->opcode_base);

	int i;
	for (i = 0; i < lm->prologue->opcode_base - 1; i++) {

		uint8_t  buf[64];

		uint32_t val = lm->prologue->standard_opcode_lengths[i];

		size_t len = scf_leb128_encode_uint32(val, buf, sizeof(buf));
		assert(len > 0);

		DWARF_DEBUG_LINE_FILL2(buf, len);
	};

	scf_dwarf_ubyte_t include_directories = 0;
	DWARF_DEBUG_LINE_FILL(include_directories);

	for (i = 0; i < lm->prologue->file_names->size; i++) {

		scf_dwarf_line_filename_t* filename = lm->prologue->file_names->data[i];

		size_t len = strlen(filename->name) + 1;
		DWARF_DEBUG_LINE_FILL2(filename->name, len);

		uint8_t  buf[64];

		len = scf_leb128_encode_uint32(filename->dir_index, buf, sizeof(buf));
		assert(len > 0);
		DWARF_DEBUG_LINE_FILL2(buf, len);

		len = scf_leb128_encode_uint32(filename->time_modified, buf, sizeof(buf));
		assert(len > 0);
		DWARF_DEBUG_LINE_FILL2(buf, len);

		len = scf_leb128_encode_uint32(filename->length, buf, sizeof(buf));
		assert(len > 0);
		DWARF_DEBUG_LINE_FILL2(buf, len);
	}

	scf_dwarf_ubyte_t end_file_names = 0;
	DWARF_DEBUG_LINE_FILL(end_file_names);

	lm->prologue->prologue_length = debug_line->len - prologue_pos - sizeof(lm->prologue->prologue_length);
	memcpy(debug_line->data + prologue_pos, &lm->prologue->prologue_length, sizeof(lm->prologue->prologue_length));

	scf_loge("lm->prologue->prologue_length: %u\n\n", lm->prologue->prologue_length);


	lm->address = 0;
	lm->file    = 1;
	lm->line    = 1;
	lm->column  = 0;
	lm->is_stmt = lm->prologue->default_is_stmt;
	lm->basic_block  = 0;
	lm->end_sequence = 0;

	for (i = 0; i < line_results->size; i++) {

		scf_dwarf_line_result_t* result = line_results->data[i];

		scf_dwarf_sword_t line_advance;
		scf_dwarf_uword_t address_advance;
		scf_dwarf_uword_t opcode;

		scf_logd("i: %d, line: %d, address: %#lx\n", i, result->line, result->address);

		uint8_t buf[64];
		size_t  len;

		if (0 == i) {
			lm->address = result->address;

			scf_dwarf_ubyte_t extended_flag   = 0;
			scf_dwarf_ubyte_t extended_opcode = DW_LNE_set_address;

			len = scf_leb128_encode_uint32(1 + sizeof(uintptr_t), buf, sizeof(buf));
			assert(len > 0);

			DWARF_DEBUG_LINE_FILL (extended_flag);
			DWARF_DEBUG_LINE_FILL2(buf, len);
			DWARF_DEBUG_LINE_FILL (extended_opcode);

			scf_rela_t* rela = calloc(1, sizeof(scf_rela_t));
			if (!rela)
				return -ENOMEM;

			rela->name = scf_string_cstr(".text");
			if (!rela->name) {
				free(rela);
				return -ENOMEM;
			}

			if (scf_vector_add(debug->line_relas, rela) < 0) {
				scf_string_free(rela->name);
				free(rela);
				return -ENOMEM;
			}
			rela->type        = R_X86_64_64;
			rela->addend      = result->address;
			rela->text_offset = debug_line->len;

			DWARF_DEBUG_LINE_FILL (lm->address);
		}

		if (result->is_stmt != lm->is_stmt) {
			lm->is_stmt     = !lm->is_stmt;

			opcode = DW_LNS_negate_stmt;
			DWARF_DEBUG_LINE_FILL2(&opcode, 1);
		}

		if (result->basic_block != lm->basic_block && result->basic_block) {
			lm->basic_block     = !lm->basic_block;

			opcode = DW_LNS_set_basic_block;
			DWARF_DEBUG_LINE_FILL2(&opcode, 1);
		}

		assert(debug->file_names->size > 0);

		scf_string_t* file_name = debug->file_names->data[lm->file - 1];

		if (strcmp(file_name->data, result->file_name->data)) {
			int j;
			for (j = 0; j < debug->file_names->size; j++) {
				file_name = debug->file_names->data[j];

				if (!strcmp(file_name->data, result->file_name->data))
					break;
			}
			assert(j < debug->file_names->size);

			lm->file = j + 1;

			opcode = DW_LNS_set_file;
			len    = scf_leb128_encode_uint32(lm->file, buf, sizeof(buf));
			assert(len > 0);
			DWARF_DEBUG_LINE_FILL2(&opcode, 1);
			DWARF_DEBUG_LINE_FILL2(buf,     len);
		}

		line_advance     = result->line    - lm->line;
		address_advance  = result->address - lm->address;

		scf_logd("result->line: %d, lm->line: %d, line_advance: %d, address_advance: %u\n",
				result->line, lm->line, line_advance, address_advance);

		assert(address_advance % lm->prologue->minimum_instruction_length == 0);

		address_advance /= lm->prologue->minimum_instruction_length;

		while (1) {
			if (line_advance - lm->prologue->line_base < lm->prologue->line_range
					&& line_advance >= lm->prologue->line_base) {

				opcode = (line_advance - lm->prologue->line_base)
					+ lm->prologue->line_range * address_advance
					+ lm->prologue->opcode_base;

				if (opcode <= 255) {

					DWARF_DEBUG_LINE_FILL2(&opcode, 1);
					scf_logd("result->line: %d, line_advance: %d, address_advance: %u, opcode: %d\n",
							result->line, line_advance, address_advance, opcode);
					break;
				}
			}

			if (line_advance != 0) {
				opcode = DW_LNS_advance_line;
				len    = scf_leb128_encode_int32(line_advance, buf, sizeof(buf));
				assert(len > 0);
				DWARF_DEBUG_LINE_FILL2(&opcode, 1);
				DWARF_DEBUG_LINE_FILL2(buf,     len);

				line_advance = 0;
				if (0 == address_advance)
					break;
				continue;
			}

			if (address_advance > 0) {
				opcode = DW_LNS_advance_pc;
				len    = scf_leb128_encode_uint32(address_advance, buf, sizeof(buf));
				assert(len > 0);
				DWARF_DEBUG_LINE_FILL2(&opcode, 1);
				DWARF_DEBUG_LINE_FILL2(buf,     len);

				address_advance = 0;
			}
		}
		lm->line    = result->line;
		lm->address = result->address;
		lm->basic_block = 0;

		if (result->end_sequence) {
			lm->end_sequence = 1;

			scf_dwarf_ubyte_t extended_flag   = 0;
			scf_dwarf_ubyte_t extended_opcode = DW_LNE_end_sequence;

			len = scf_leb128_encode_uint32(1, buf, sizeof(buf));
			assert(len > 0);

			DWARF_DEBUG_LINE_FILL (extended_flag);
			DWARF_DEBUG_LINE_FILL2(buf, len);
			DWARF_DEBUG_LINE_FILL (extended_opcode);

			lm->address = 0;
			lm->file    = 1;
			lm->line    = 1;
			lm->column  = 0;
			lm->is_stmt = lm->prologue->default_is_stmt;
			lm->basic_block  = 0;
			lm->end_sequence = 0;
		}
	}

	lm->prologue->total_length = debug_line->len - origin_len - sizeof(lm->prologue->total_length);
	memcpy(debug_line->data + origin_len, &lm->prologue->total_length, sizeof(lm->prologue->total_length));

	scf_loge("lm->prologue->total_length: %u\n", lm->prologue->total_length);

	return 0;
}

