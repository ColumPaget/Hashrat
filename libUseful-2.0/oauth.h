#ifndef LIBUSEFUL_OAUTH_H
#define LIBUSEFUL_OAUTH_H

#ifdef __cplusplus
extern "C" {
#endif

void OAuthDeviceLogin(char *LoginURL, char *ClientID, char *Scope, char **DeviceCode, char **UserCode, char **NextURL);
void OAuthDeviceGetAccessToken(char *TokenURL, char *ClientID, char *ClientSecret, char *DeviceCode, char **AccessToken, char **RefreshToken);
void OAuthDeviceRefreshToken(char *TokenURL, char *ClientID, char *ClientSecret, char *RefreshToken, char **AccessToken);
void OAuthInstalledAppURL(char *LoginURL, char *ClientID, char *Scope, char *RedirectURL, char **NextURL);
void OAuthInstalledAppGetAccessToken(char *TokenURL, char *ClientID, char *ClientSecret, char *AuthCode, char *RedirectURL, char **AccessToken, char **RefreshToken);

#ifdef __cplusplus
}
#endif

#endif

