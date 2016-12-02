# Heat pump acl functionality

# unpaired fresh mode

1. Client connects locally. Connection access is granted since device is unpaired.
2. Client calls getAclInfo.json


# paired device local access

1. Client connects locally. Connection access is granted since the connection is local.


## me.json / getPublicInfo.json

request which tells what state current client is in 

{
   ...  oldGetPublicInfo
  "paired": 0|1
}


# user model on the device

The device has a list of users identified by the fignerprint. If a user is paired his fingerprint is in the list of known users.

[user]
 
 struct user {
   uint8_t fingerprint[16]
   char userName[64]
   uint32_t permissions
 }
 
 
# pairing mode

The app has a button which owners can toggle to enable/disable pairing mode.
 
## setPairingMode.json
{
	"localPairing": 1
}

## getPairingMode.json
{
  "localPairing": 0
  "remotePairing": 0
}

# users

A user is identified by his fingerprint.

## getUsers.json

request:
maxUsersPerRequest: uint8_t 
optional startFingerprint: uint8_t[16] // used in pagination mode

{
  "users": [
    { "userName": string
	  "fingerprint": uint8_t[16]
	  "permissions": uint32_t
    }
  ]
  "next": uint8_t[16] // fingerprint to use as startFingerprint if more pages are available
}

## removeUser.json

request:
userid: fingerprint

response:
{
  "status": ACL_OK | ACL_FAILED
}

## addMe.json

{
  "userName": ...
  
}

## getMe.json
{
  "userName": string
  "fingerprint": uint8_t[16]
  "permissions": uint32_t
  "paired": uint8_t (0|1)
}

## getUser.json

request:
fingerprint: uint8_t[16]

response:
{
  "userName": string
  "fingerprint": uint8_t[16]
  "permissions": uint32_t
}


## addPermissions.json

request:
fingerprint: uint8_t[16]
permissions: uint32_t;

response:
{
    "permissions": uint32_t new aggregated permissions bits
}

## removePermissions.json

request:
fingerprint: uint8_t[16]
permissions: uint32_t

response:
{
  "permissions": uint32_t new aggregated permissions bits.
}

## setUserName.json
request:
fingerprint: uint8_t[16]
userName: string

response:
{
  userName: string, //the userName as it was saved on the device
}
