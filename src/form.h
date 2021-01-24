#ifndef H_KORE_FORM
#define H_KORE_FORM

struct kore_form
{
	struct kore_buf* buf;
	size_t numElem;
	CURL* handle;
};

struct kore_form* kore_form_init(CURL* handle);
void kore_form_free(struct kore_form* form);

void kore_form_add(struct kore_form* form, const char* name, const char* value, size_t len);
char* kore_form_stringify(struct kore_form* form, size_t* len);
void kore_form_post(struct kore_form* form);

#endif