/*
 * Copyright (c) 2021 Benoist André
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <kore/kore.h>
#include <kore/curl.h>

#include "form.h"

struct kore_form* kore_form_init(CURL* handle)
{
	struct kore_form* form = kore_malloc(sizeof(struct kore_form));
	if (form)
	{
		struct kore_buf *buf = kore_buf_alloc(256);
		if (!buf)
		{
			kore_free(form);
			return NULL;
		}
		form->buf = buf;
		form->numElem = 0;
		form->handle = handle;
	}
	return form;
}

void kore_form_free(struct kore_form* form)
{
	kore_buf_free(form->buf);
	kore_free(form);
}

void kore_form_add(struct kore_form* form, const char* name, const char* value, size_t len)
{
	char* escaped = curl_easy_escape(form->handle, value, len);
	if (form->numElem)
		kore_buf_appendf(form->buf, "&%s=%s", name, escaped);
	else
		kore_buf_appendf(form->buf, "%s=%s", name, escaped);
	form->numElem++;
	curl_free(escaped);
}

char* kore_form_stringify(struct kore_form* form, size_t* len)
{
	return kore_buf_stringify(form->buf, len);
}

void kore_form_post(struct kore_form* form)
{
	size_t len;
	char *fields = kore_form_stringify(form, &len);
	
	curl_easy_setopt(form->handle, CURLOPT_POSTFIELDSIZE, len);
	curl_easy_setopt(form->handle, CURLOPT_POSTFIELDSIZE_LARGE, len);
	curl_easy_setopt(form->handle, CURLOPT_COPYPOSTFIELDS, fields);
}