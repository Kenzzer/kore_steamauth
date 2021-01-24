#ifndef H_KORE_STEAMAUTH
#define H_KORE_STEAMAUTH

struct steam_auth
{
	struct kore_curl curl;
	u_int64_t steamid;
};

// Validates the openID GET parameters according to STEAM
// Returns 0 on success - 1 on failure - 2 internal error
int steam_validate_auth(struct http_request* req);

#endif