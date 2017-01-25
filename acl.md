# Access Control functionality in demo app 

## Operating modes

The device can either be open for local pairing or closed for local
pairing, if it's open for local pairing then it's possible to pair
with the device if it is closed for local pairing it is not possible
to pair with the device. When a user pairs with the device the
permission granted is controlled by the admin of the device.


## Unpaired fresh mode

1. Client connects locally. Connection access is granted since device is unpaired. [AMP-68, AMP-71]
2. Client calls getPublicInfo.json to find out if you are paired with the device. [AMP-68]
3. Client goes into pairing mode and calls pairWithDevice.json [AMP-69]
4. Client is granted owner permissions to the device


## Paired device local pairing access

1. Client connects locally. Connection access is granted since the connection is local. [AMP-68, AMP-71]
2. Client calls getPublicInfo.json to find out that it is not paired. [AMP-68]
3. Client goes into pairing mode and calls pairWithDevice.json [AMP-68, AMP-69, AMP-71]
4. Client is granted guest permissions since the device already have an owner.

## Access device you are paired with

1. Client connects to the device.
2. Client calls getPublicInfo.json to ensure that the client is paired with the device.

## Remote Access to a device where you have been removed from the ACL
1. The device is in the list of known devices
2. Client connects to the device. The connection is not granted and fails with ACCESS_DENIED
3. The user is informed about the situation and asked to re-pair with the device.

## Local Access to a device where you have been removed from the ACL
1. the device is in the list of known devices
2. Client connects to the device, the connection is granted because the device is in local pairing mode.
3. Client calls getPublicInfo.json and discovers that it is not seen as paired.
4. Client goes into pairing mode and calls pairWithDevice.json

## getPublicInfo.json

request which tells what state current client is in 
```
{
   ...  oldGetPublicInfo
  "paired": (UNPAIRED = 0 | PAIRED = 1)
  "pairingMode": (CLOSED_FOR_PAIRING = 0 | OPEN_FOR_PAIRING = 1 )
}
```

## User model on the device

The device has a list of users identified by the fignerprint. If a user is paired his fingerprint is in the list of known users.

```
users: [user]
```

```
struct user {
  uint8_t fingerprint[16]
  char userName[64]
  uint32_t permissions
}
```

acl settings
```
struct aclSettings {
  bool openForPairing,
  uint32_t defaultPairingPermissions
}
```
 
## Pairing mode

The app has a button which owners can toggle to enable/disable pairing mode. Only owners can toggle pairing mode.
 
### setLocalPairingMode.json

```
{
  "localPairing": 1
}
```

### getLocalPairingMode.json
```
{
  "localPairing": 0
}
```

### setLocalPairingPermissions.json [AMP-70]

What is the default permissions a user grants after being paired with
the device locally, this call requires admin permissions.

```
{
  "permissions": uint32_t mask (LOCAL_ACCESS | REMOTE_ACCESS | ADMIN)
}
```

### getLocalPairingPermissions.json
```
{
  "permissions": uint32_t mask (LOCAL_ACCESS | REMOTE_ACCESS | ADMIN)
}
```


## users

A user is identified by his fingerprint.

### getUsers.json

All users can get all users.

request:
```
maxUsersPerRequest: uint8_t 
optional startFingerprint: uint8_t[16] // used in pagination mode
```

```
{
  "users": [
    { "userName": string
      "fingerprint": uint8_t[16]
      "permissions": uint32_t
    }
  ]
  "next": uint8_t[16] // fingerprint to use as startFingerprint if more pages are available
}
```

### removeUser.json

Owners can remove all other users including themselves.
Guests can only remove themselves.

request:
```
userid: fingerprint
```

response:
```
{
  "status": ACL_OK | ACL_FAILED
}
```

### pairWithDevice.json

if this is the first one to pair with the device this person will be granted owner access. If the device already have paired users, this person will be granted guest access.

request:
```
{
  "userName": ...
}
```

response:
```
{
  "userName": string,
  "fingerprint": uint8_t[16]
  "permissions": uint32_t
}
```
  
### getMe.json

return a description of the user you are logged in as.

```
{
  "userName": string
  "fingerprint": uint8_t[16]
  "permissions": uint32_t
  "paired": uint8_t (0|1)
}
```
  
### getUser.json

All users can get all the other users permissions.

get a user identified by the fingerprint

request:
```
fingerprint: uint8_t[16]
```
  
response:
```
{
  "userName": string
  "fingerprint": uint8_t[16]
  "permissions": uint32_t
}
```
  
### addPermissions.json

Owners can change permissions of all other users including themselves

add permission bits to the permissions for a given user

request:
```
fingerprint: uint8_t[16]
permissions: uint32_t;
```
 
response:
```
{
  "permissions": uint32_t new aggregated permissions bits
}
```
  
### removePermissions.json

remove permission bits from a given user.

request:
```
fingerprint: uint8_t[16]
permissions: uint32_t
```

response:
```
{
  "permissions": uint32_t new aggregated permissions bits.
}
```
  
### setUserName.json

Set the username of a user and return the username as it was saved on the device (it could have been truncated)

Owners can change names of all users, guests can only change names of themselves.

request:
```
fingerprint: uint8_t[16]
userName: string
```
  
response:
```
{
  userName: string
}
```
