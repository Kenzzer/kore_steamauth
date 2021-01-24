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
#include "steamauth.h"

// Validates the openID GET parameters according to STEAM
// Returns 0 on success - 1 on failure - 2 internal error
int steam_validate_auth(struct http_request* req)
{
	// Retrieve all the parameters
	char* mode = NULL, *ns = NULL, *returnto = NULL, *endpoint = NULL, *response_nonce = NULL, *assoc_handle = NULL, *signe = NULL, *sig = NULL, *identity = NULL, *claimed = NULL;
	if (http_argument_get_string(req, "openid.mode", &mode) != (KORE_RESULT_OK)
	|| http_argument_get_string(req, "openid.ns", &ns) != (KORE_RESULT_OK)
	|| http_argument_get_string(req, "openid.return_to", &returnto) != (KORE_RESULT_OK)
	|| http_argument_get_string(req, "openid.op_endpoint", &endpoint) != (KORE_RESULT_OK)
	|| http_argument_get_string(req, "openid.response_nonce", &response_nonce) != (KORE_RESULT_OK)
	|| http_argument_get_string(req, "openid.assoc_handle", &assoc_handle) != (KORE_RESULT_OK)
	|| http_argument_get_string(req, "openid.signed", &signe) != (KORE_RESULT_OK)
	|| http_argument_get_string(req, "openid.sig", &sig) != (KORE_RESULT_OK)
	|| http_argument_get_string(req, "openid.claimed_id", &identity) != (KORE_RESULT_OK)
	|| http_argument_get_string(req, "openid.identity", &claimed) != (KORE_RESULT_OK))
	{
		return 1;
	}
	
	// Identity & Claimed ID should always be the same
	if (strcmp(identity, claimed) != 0)
	{
		return 1;
	}
	
	// You may optionally wanna check where "openid.return_to" points on your page or not
	
	// Retrieve the steamid
	int err = KORE_RESULT_OK;
	u_int64_t steamid = kore_strtonum64((identity + 37), 0, &err);
	// (identity + 37) Skips "https://steamcommunity.com/openid/id/"
	// this would be bad if the string had a length smaller than 37 bytes
	// however parameters have been validated by Kore with a regex check
	if (err != KORE_RESULT_OK || steamid == 0)
	{
		return 1;
	}

	// Create a CURL request
	struct steam_auth *login;
	login = http_state_create(req, sizeof(*login), NULL);
	login->steamid = steamid;
	
	// We can't validate the steamid if we can't query steam
	if (kore_curl_init(&login->curl, "https://steamcommunity.com/openid/login", KORE_CURL_ASYNC) != (KORE_RESULT_OK))
	{
		return 2;
	}
	kore_curl_http_setup(&login->curl, HTTP_METHOD_POST, NULL, 0);
	
	// Setup our POST form
	CURL* handle = login->curl.handle;
	struct kore_form *form = kore_form_init(handle);
	if (!form)
	{
		kore_curl_cleanup(&login->curl);
		return 2;
	}
	
	kore_form_add(form, "openid.ns", ns, strlen(ns));
	kore_form_add(form, "openid.mode", "check_authentication", 20);
	kore_form_add(form, "openid.op_endpoint", endpoint, strlen(endpoint));
	kore_form_add(form, "openid.claimed_id", claimed, strlen(claimed));
	kore_form_add(form, "openid.identity", identity, strlen(identity));
	kore_form_add(form, "openid.return_to", returnto, strlen(returnto));
	kore_form_add(form, "openid.response_nonce", response_nonce, strlen(response_nonce));
	kore_form_add(form, "openid.assoc_handle", assoc_handle, strlen(assoc_handle));
	kore_form_add(form, "openid.signed", signe, strlen(signe));
	kore_form_add(form, "openid.sig", sig, strlen(sig));
	kore_form_post(form);
	
	curl_easy_setopt(handle, CURLOPT_USERAGENT, "OpenID Verification (https://github.com/Kenzzer/kore_steamauth)");
	curl_easy_setopt(handle, CURLOPT_TIMEOUT, 6);
	curl_easy_setopt(handle, CURLOPT_CONNECTTIMEOUT, 6);
	
	kore_curl_bind_request(&login->curl, req);
	kore_curl_run(&login->curl);
	
	kore_form_free(form);
	return 0;
}