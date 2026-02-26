HTTP Request (Appwrite SDK)
===========================

Base Request
------------

```
POST /v1/<url> HTTP/1.1
Conent-Type: application/json
X-Appwrite-Project: <projectId>
X-Appwrite-Key: <apiKey>         // only for admin access
```

Create Anon Session
---------------------

```
POST /v1/account/sessions/anonymous HTTP/1.1
Conent-Type: application/json
X-Appwrite-Project: <projectId>
```

Create Email Session
----------------------

```
POST /v1/account/sessions/email HTTP/1.1
Conent-Type: application/json
X-Appwrite-Project: <projectId>

{
    "email": <email>,
    "password": <password>
}
```

Create Document
-----------------

```
POST /v1/databases/<dbId>/collections/<collectionId>/documents HTTP/1.1
Content-Type: application/json
X-Appwrite-Project: <projectId>

{
    "documentId": "unique()",
    "data": <data>
}
```

Create Database
-----------------

```
POST /v1/databases HTTP/1.1
Content-Type: application/json
X-Appwrite-Project: <projectId>
X-Appwrite-Key: <apiKey>

{
    "databaseId": <dbId>,
    "name": <name>
}
```

Create Collection
-------------------

```
POST /v1/databases/<dbId>/collections HTTP/1.1
Content-Type: application/json
X-Appwrite-Project: <projectId>
X-Appwrite-Key: <apiKey>

{
    "collectionId": <collectionId>,
    "name": <name>,
    "permissions": [
        "read("any")",
        "write("any")"
    ]
}
```

Create String Attribute
-------------------------

```
POST /v1/databases/<dbId>/collections HTTP/1.1
Content-Type: application/json
X-Appwrite-Project: <projectId>
X-Appwrite-Key: <apiKey>

{
    "key": <key>,
    "size": <size>,
    "required": <required>
}
```

Create User
--------------

```
POST /v1/databases/<dbId>/collections HTTP/1.1
Content-Type: application/json
X-Appwrite-Project: <projectId>
X-Appwrite-Key: <apiKey>

{
    "userId": "unique()",
    "email": <email>,
    "password": <password>
}
```
