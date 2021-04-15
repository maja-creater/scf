#include"scf_dwarf_def.h"
#include"scf_leb128.h"

scf_dwarf_info_entry_t* scf_dwarf_info_entry_alloc()
{
	scf_dwarf_info_entry_t* ie = calloc(1, sizeof(scf_dwarf_info_entry_t));
	if (!ie)
		return NULL;

	ie->attributes = scf_vector_alloc();
	if (!ie->attributes) {
		free(ie);
		return NULL;
	}

	return ie;
}

void scf_dwarf_info_attr_free(scf_dwarf_info_attr_t* attr)
{
	if (attr) {
		if (attr->data)
			scf_string_free(attr->data);

		free(attr);
	}
}

void scf_dwarf_info_entry_free(scf_dwarf_info_entry_t* ie)
{
	if (ie) {
		if (ie->attributes) {
			scf_vector_clear(ie->attributes, (void (*)(void*)) scf_dwarf_info_attr_free);
			scf_vector_free(ie->attributes);
		}

		free(ie);
	}
}

static int _abbrev_find_code(const void* v0, const void* v1)
{
	const scf_dwarf_uword_t               code = (uintptr_t)v0;

	const scf_dwarf_abbrev_declaration_t* d    = v1;

	return code != d->code; 
}

int scf_dwarf_info_decode(scf_vector_t* infos, scf_vector_t* abbrevs, scf_string_t* debug_str, const char*   debug_info, size_t debug_info_size, scf_dwarf_info_header_t* header)
{
	if (!infos || !abbrevs || !debug_str || !debug_info || 0 == debug_info_size || !header) {
		scf_loge("debug_info_size: %ld\n", debug_info_size);
		return -EINVAL;
	}

	if (debug_info_size < sizeof(scf_dwarf_uword_t)) {
		scf_loge("\n");
		return -EINVAL;
	}

	header->length = *(scf_dwarf_uword_t*)debug_info;
	size_t i  = sizeof(scf_dwarf_uword_t);

	scf_loge("header->length: %d\n", header->length);

	if (i + header->length < debug_info_size) {
		scf_loge("\n");
		return -EINVAL;
	}

	assert(i +   sizeof(scf_dwarf_uhalf_t) < debug_info_size);
	header->version = *(scf_dwarf_uhalf_t*)( debug_info + i);
	i +=         sizeof(scf_dwarf_uhalf_t);

	assert(i +  sizeof(scf_dwarf_uword_t) < debug_info_size);
	header->offset = *(scf_dwarf_uword_t*)( debug_info + i);
	i +=        sizeof(scf_dwarf_uword_t);

	assert(i +        sizeof(scf_dwarf_ubyte_t) <= debug_info_size);
	header->address_size = *(scf_dwarf_ubyte_t*)(  debug_info + i);
	i +=              sizeof(scf_dwarf_ubyte_t);

	scf_dwarf_abbrev_declaration_t* d    = NULL;
	scf_dwarf_abbrev_attribute_t*   attr = NULL;
	scf_dwarf_info_entry_t*         ie   = NULL;
	scf_dwarf_info_attr_t*          iattr = NULL;

	while (i < debug_info_size) {

		ie = scf_dwarf_info_entry_alloc();
		if (!ie)
			return -ENOMEM;
		ie->cu_byte_offset = i;

		int ret = scf_vector_add(infos, ie);
		if (ret < 0) {
			scf_dwarf_info_entry_free(ie);
			return ret;
		}

		size_t len;

		len = 0;
		ie->code = scf_leb128_decode_uint32(debug_info + i, &len);
		assert(len > 0);
		i += len;
		assert(i <= debug_info_size);

		d = scf_vector_find_cmp(abbrevs, (void*)(uintptr_t)ie->code, _abbrev_find_code);
		if (!d)
			return -1;

		int k;
		for (k = 0; k < d->attributes->size; k++) {
			attr      = d->attributes->data[k];

			iattr = calloc(1, sizeof(scf_dwarf_info_attr_t));
			if (!iattr)
				return -ENOMEM;

			ret = scf_vector_add(ie->attributes, iattr);
			if (ret < 0) {
				scf_dwarf_info_attr_free(iattr);
				return ret;
			}

			iattr->name = attr->name;
			iattr->form = attr->form;

			int j;

			switch (iattr->form) {

				case DW_FORM_addr:
					assert(i + sizeof(uintptr_t) <= debug_info_size);

					iattr->address = *(uintptr_t*)(debug_info + i);
					i += sizeof(uintptr_t);
					break;

				case DW_FORM_data1:
					assert(i + sizeof(uint8_t) <= debug_info_size);

					iattr->const1 = *(uint8_t*)(debug_info + i);
					i += sizeof(uint8_t);
					break;
				case DW_FORM_data2:
					assert(i + sizeof(uint16_t) <= debug_info_size);

					iattr->const2 = *(uint16_t*)(debug_info + i);
					i += sizeof(uint16_t);
					break;
				case DW_FORM_data4:
					assert(i + sizeof(uint32_t) <= debug_info_size);

					iattr->const4 = *(uint32_t*)(debug_info + i);
					i += sizeof(uint32_t);
					break;
				case DW_FORM_data8:
					assert(i + sizeof(uint64_t) <= debug_info_size);

					iattr->const8 = *(uint64_t*)(debug_info + i);
					i += sizeof(uint64_t);
					break;

				case DW_FORM_sdata:
					len = 0;
					iattr->sdata = scf_leb128_decode_int32(debug_info + i, &len);
					assert(len > 0);

					i += len;
					assert(i <= debug_info_size);
					break;
				case DW_FORM_udata:
					len = 0;
					iattr->udata = scf_leb128_decode_uint32(debug_info + i, &len);
					assert(len > 0);

					i += len;
					assert(i <= debug_info_size);
					break;

				case DW_FORM_flag:
					assert(i < debug_info_size);

					iattr->flag  = debug_info[i++];
					break;
				case DW_FORM_flag_present:
					iattr->flag  = 1;
					break;

				case DW_FORM_ref1:
					assert(i + sizeof(uint8_t) <= debug_info_size);

					iattr->ref8 = *(uint8_t*)(debug_info + i);
					i += sizeof(uint8_t);
					break;
				case DW_FORM_ref2:
					assert(i + sizeof(uint16_t) <= debug_info_size);

					iattr->ref8 = *(uint16_t*)(debug_info + i);
					i += sizeof(uint16_t);
					break;
				case DW_FORM_ref4:
					assert(i + sizeof(uint32_t) <= debug_info_size);

					iattr->ref8 = *(uint32_t*)(debug_info + i);
					i += sizeof(uint32_t);
					break;
				case DW_FORM_ref8:
					assert(i + sizeof(uint64_t) <= debug_info_size);

					iattr->ref8 = *(uint64_t*)(debug_info + i);
					i += sizeof(uint64_t);
					break;
				case DW_FORM_ref_udata:
					len = 0;
					iattr->ref8 = scf_leb128_decode_uint32(debug_info + i, &len);
					assert(len > 0);

					i += len;
					assert(i <= debug_info_size);
					break;

				case DW_FORM_string:
					j = i;
					while (j < debug_info_size && debug_info[j])
						j++;
					assert(j < debug_info_size);

					iattr->data = scf_string_cstr_len(debug_info + i, j - i);
					if (!iattr->data)
						return -ENOMEM;

					i = j + 1;
					break;
				case DW_FORM_strp:
					assert(i + sizeof(scf_dwarf_uword_t) <= debug_info_size);

					iattr->str_offset = *(scf_dwarf_uword_t*)(debug_info + i);
					i += sizeof(scf_dwarf_uword_t);

					j = iattr->str_offset;
					while (j < debug_str->len && debug_str->data[j])
						j++;
					assert(j <= debug_str->len);
					assert('\0' == debug_str->data[j]);

					iattr->data = scf_string_cstr_len(debug_str->data + iattr->str_offset, j - iattr->str_offset);
					if (!iattr->data)
						return -ENOMEM;
					break;

				case DW_FORM_sec_offset:
					assert(i + sizeof(scf_dwarf_uword_t) <= debug_info_size);

					iattr->lineptr = *(scf_dwarf_uword_t*)(debug_info + i);
					i += sizeof(scf_dwarf_uword_t);

					break;

				case DW_FORM_exprloc:
					len = 0;
					iattr->exprloc = scf_leb128_decode_uint32(debug_info + i, &len);
					assert(len > 0);

					i += len;
					assert(i + iattr->exprloc <= debug_info_size);

					iattr->data = scf_string_cstr_len(debug_info + i, iattr->exprloc);
					if (!iattr->data)
						return -ENOMEM;

					i += iattr->exprloc;
					break;
				case 0:
					assert(0 == iattr->name);
					break;
				default:
					scf_loge("iattr->form: %u\n", iattr->form);
					scf_loge("iattr->form: %s\n", scf_dwarf_find_form(iattr->form));
					return -1;
					break;
			};
		}
	}

	return 0;
}

int scf_dwarf_info_encode(scf_vector_t* infos, scf_vector_t* abbrevs, scf_string_t* debug_str, scf_string_t* debug_info, scf_dwarf_info_header_t* header)
{
	if (!infos || !abbrevs || !debug_str || !debug_info || !header) {
		scf_loge("\n");
		return -EINVAL;
	}

#define DWARF_INFO_FILL(str, buf, len) \
	do { \
		int ret = scf_string_cat_cstr_len(str, (char*)buf, len); \
		if (ret < 0) { \
			scf_loge("\n"); \
			return ret; \
		} \
	} while (0)

	size_t origin_len = debug_info->len;

	DWARF_INFO_FILL(debug_info, &header->length,       sizeof(header->length));
	DWARF_INFO_FILL(debug_info, &header->version,      sizeof(header->version));
	DWARF_INFO_FILL(debug_info, &header->offset,       sizeof(header->offset));
	DWARF_INFO_FILL(debug_info, &header->address_size, sizeof(header->address_size));

	scf_dwarf_abbrev_declaration_t* d     = NULL;
	scf_dwarf_abbrev_attribute_t*   attr  = NULL;
	scf_dwarf_info_entry_t*         ie    = NULL;
	scf_dwarf_info_attr_t*          iattr = NULL;

	int i;
	for (i = 0; i < infos->size; i++) {
		ie =        infos->data[i];

		uint8_t buf[64];
		size_t  len;

		len = scf_leb128_encode_uint32(ie->code, buf, sizeof(buf));
		assert(len > 0);
		DWARF_INFO_FILL(debug_info, buf, len);

		d = scf_vector_find_cmp(abbrevs, (void*)(uintptr_t)ie->code, _abbrev_find_code);
		if (!d)
			return -1;

		if (d->attributes->size != ie->attributes->size) {
			scf_loge("\n");
			return -1;
		}

		int k;
		for (k = 0; k < ie->attributes->size; k++) {
			iattr     = ie->attributes->data[k];

			int j;

			switch (iattr->form) {

				case DW_FORM_addr:
					DWARF_INFO_FILL(debug_info, &iattr->address, sizeof(uintptr_t));
					break;

				case DW_FORM_data1:
					DWARF_INFO_FILL(debug_info, &iattr->const1, sizeof(uint8_t));
					break;
				case DW_FORM_data2:
					DWARF_INFO_FILL(debug_info, &iattr->const2, sizeof(uint16_t));
					break;
				case DW_FORM_data4:
					DWARF_INFO_FILL(debug_info, &iattr->const4, sizeof(uint32_t));
					break;
				case DW_FORM_data8:
					DWARF_INFO_FILL(debug_info, &iattr->const8, sizeof(uint64_t));
					break;

				case DW_FORM_sdata:
					len = scf_leb128_encode_int32(iattr->sdata, buf, sizeof(buf));
					assert(len > 0);
					DWARF_INFO_FILL(debug_info, buf, len);
					break;
				case DW_FORM_udata:
					len = scf_leb128_encode_uint32(iattr->udata, buf, sizeof(buf));
					assert(len > 0);
					DWARF_INFO_FILL(debug_info, buf, len);
					break;

				case DW_FORM_flag:
					DWARF_INFO_FILL(debug_info, &iattr->flag, 1);
					break;
				case DW_FORM_flag_present:
					break;

				case DW_FORM_ref1:
					DWARF_INFO_FILL(debug_info, &iattr->ref8, sizeof(uint8_t));
					break;
				case DW_FORM_ref2:
					DWARF_INFO_FILL(debug_info, &iattr->ref8, sizeof(uint16_t));
					break;
				case DW_FORM_ref4:
					DWARF_INFO_FILL(debug_info, &iattr->ref8, sizeof(uint32_t));
					break;
				case DW_FORM_ref8:
					DWARF_INFO_FILL(debug_info, &iattr->ref8, sizeof(uint64_t));
					break;
				case DW_FORM_ref_udata:
					len = scf_leb128_encode_uint32(iattr->ref8, buf, sizeof(buf));
					assert(len > 0);
					DWARF_INFO_FILL(debug_info, buf, len);
					break;

				case DW_FORM_string:
					DWARF_INFO_FILL(debug_info, iattr->data->data, iattr->data->len + 1);
					break;
				case DW_FORM_strp:
					DWARF_INFO_FILL(debug_info, &iattr->str_offset, sizeof(scf_dwarf_uword_t));
					break;

				case DW_FORM_sec_offset:
					DWARF_INFO_FILL(debug_info, &iattr->lineptr, sizeof(scf_dwarf_uword_t));
					break;

				case DW_FORM_exprloc:
					len = scf_leb128_encode_uint32(iattr->exprloc, buf, sizeof(buf));
					assert(len > 0);
					DWARF_INFO_FILL(debug_info, buf, len);

					DWARF_INFO_FILL(debug_info, iattr->data->data, iattr->data->len);
					break;
				case 0:
					assert(0 == iattr->name);
					break;
				default:
					scf_loge("iattr->form: %u\n", iattr->form);
					scf_loge("iattr->form: %s\n", scf_dwarf_find_form(iattr->form));
					return -1;
					break;
			};
		}
	}

	return 0;
}

void scf_dwarf_info_print(scf_vector_t* infos)
{
	if (!infos)
		return;

	scf_dwarf_info_entry_t* ie;
	scf_dwarf_info_attr_t*  iattr;

	int i;
	for (i = 0; i < infos->size; i++) {
		ie =        infos->data[i];

		printf("code: %u, ie->attributes->size: %d\n", ie->code, ie->attributes->size);

		int j;
		for (j = 0; j < ie->attributes->size; j++) {
			iattr     = ie->attributes->data[j];

			if (0 == iattr->name || 0 == iattr->form) {

				printf("%u, %u, ", iattr->name, iattr->form);
			} else {
				int len = printf("%s, ",scf_dwarf_find_attribute(iattr->name));
				len = 30 - len;

				char fmt[16];
				sprintf(fmt, "%%%dc", len);
				printf(fmt, ' ');

				printf("%s, ", scf_dwarf_find_form(iattr->form));
			}

			switch (iattr->form) {

				case DW_FORM_addr:
					printf("%#lx\n", iattr->address);
					break;

				case DW_FORM_data1:
					printf("%u\n", iattr->const1);
					break;
				case DW_FORM_data2:
					printf("%u\n", iattr->const2);
					break;
				case DW_FORM_data4:
					printf("%u\n", iattr->const4);
					break;
				case DW_FORM_data8:
					printf("%lu\n", iattr->const8);
					break;

				case DW_FORM_sdata:
					printf("%d\n", iattr->sdata);
					break;
				case DW_FORM_udata:
					printf("%u\n", iattr->udata);
					break;

				case DW_FORM_flag:
				case DW_FORM_flag_present:
					printf("%u\n", iattr->flag);
					break;

				case DW_FORM_ref1:
				case DW_FORM_ref2:
				case DW_FORM_ref4:
				case DW_FORM_ref8:
				case DW_FORM_ref_udata:
					printf("%lu\n", iattr->ref8);
					break;

				case DW_FORM_string:
				case DW_FORM_strp:
					printf("%s\n", iattr->data->data);
					break;

				case DW_FORM_sec_offset:
					printf("%u\n", iattr->lineptr);
					break;

				case DW_FORM_exprloc:
					printf("%u\n", iattr->exprloc);
					break;
				case 0:
					printf("%u, %u\n", iattr->name, iattr->form);
					break;
				default:
					printf("%u, %u\n", iattr->name, iattr->form);
					break;
			};
		}
	}
}

