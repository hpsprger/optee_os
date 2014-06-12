/*
 * Copyright (c) 2014, STMicroelectronics International N.V.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <stdlib.h>
#include <string.h>

#include <tee_api.h>
#include <utee_syscalls.h>

#include <assert.h>

#define TEE_USAGE_DEFAULT   0xffffffff

#define TEE_ATTR_BIT_VALUE                  (1 << 29)
#define TEE_ATTR_BIT_PROTECTED              (1 << 28)

/* Data and Key Storage API  - Generic Object Functions */
void TEE_GetObjectInfo(TEE_ObjectHandle object, TEE_ObjectInfo *objectInfo)
{
	TEE_Result res;

	res = utee_cryp_obj_get_info((uint32_t)object, objectInfo);
	if (res != TEE_SUCCESS)
		TEE_Panic(res);
}

void TEE_RestrictObjectUsage(TEE_ObjectHandle object, uint32_t objectUsage)
{
	TEE_Result res;
	res = utee_cryp_obj_restrict_usage((uint32_t)object, objectUsage);

	if (res != TEE_SUCCESS)
		TEE_Panic(0);
}

TEE_Result TEE_GetObjectBufferAttribute(TEE_ObjectHandle object,
					uint32_t attributeID, void *buffer,
					size_t *size)
{
	TEE_Result res;
	TEE_ObjectInfo info;

	res = utee_cryp_obj_get_info((uint32_t)object, &info);
	if (res != TEE_SUCCESS)
		TEE_Panic(0);

	if ((info.handleFlags & TEE_HANDLE_FLAG_INITIALIZED) == 0)
		TEE_Panic(0);

	/* This function only supports reference attributes */
	if ((attributeID & TEE_ATTR_BIT_VALUE) != 0)
		TEE_Panic(0);

	res =
	    utee_cryp_obj_get_attr((uint32_t)object, attributeID, buffer,
				   size);

	if (res != TEE_SUCCESS && res != TEE_ERROR_ITEM_NOT_FOUND &&
	    res != TEE_ERROR_SHORT_BUFFER)
		TEE_Panic(0);

	return res;
}

TEE_Result TEE_GetObjectValueAttribute(TEE_ObjectHandle object,
				       uint32_t attributeID, uint32_t *a,
				       uint32_t *b)
{
	TEE_Result res;
	TEE_ObjectInfo info;
	uint32_t buf[2];
	size_t size = sizeof(buf);

	res = utee_cryp_obj_get_info((uint32_t)object, &info);
	if (res != TEE_SUCCESS)
		TEE_Panic(0);

	if ((info.handleFlags & TEE_HANDLE_FLAG_INITIALIZED) == 0)
		TEE_Panic(0);

	/* This function only supports value attributes */
	if ((attributeID & TEE_ATTR_BIT_VALUE) == 0)
		TEE_Panic(0);

	res =
	    utee_cryp_obj_get_attr((uint32_t)object, attributeID, buf, &size);

	if (res != TEE_SUCCESS && res != TEE_ERROR_ITEM_NOT_FOUND &&
	    res != TEE_ERROR_ACCESS_DENIED)
		TEE_Panic(0);

	if (size != sizeof(buf))
		TEE_Panic(0);

	*a = buf[0];
	*b = buf[1];

	return res;
}

void TEE_CloseObject(TEE_ObjectHandle object)
{
	TEE_Result res;

	if (object == TEE_HANDLE_NULL)
		return;

	res = utee_cryp_obj_close((uint32_t)object);
	if (res != TEE_SUCCESS)
		TEE_Panic(0);
}

/* Data and Key Storage API  - Transient Object Functions */

TEE_Result TEE_AllocateTransientObject(TEE_ObjectType objectType,
				       uint32_t maxObjectSize,
				       TEE_ObjectHandle *object)
{
	TEE_Result res;
	uint32_t obj;

	res = utee_cryp_obj_alloc(objectType, maxObjectSize, &obj);
	if (res == TEE_SUCCESS)
		*object = (TEE_ObjectHandle) obj;
	return res;
}

void TEE_FreeTransientObject(TEE_ObjectHandle object)
{
	TEE_Result res;
	TEE_ObjectInfo info;

	if (object == TEE_HANDLE_NULL)
		return;

	res = utee_cryp_obj_get_info((uint32_t)object, &info);
	if (res != TEE_SUCCESS)
		TEE_Panic(0);

	if ((info.handleFlags & TEE_HANDLE_FLAG_PERSISTENT) != 0)
		TEE_Panic(0);

	res = utee_cryp_obj_close((uint32_t)object);
	if (res != TEE_SUCCESS)
		TEE_Panic(0);
}

void TEE_ResetTransientObject(TEE_ObjectHandle object)
{
	TEE_Result res;
	TEE_ObjectInfo info;

	if (object == TEE_HANDLE_NULL)
		return;

	res = utee_cryp_obj_get_info((uint32_t)object, &info);
	if (res != TEE_SUCCESS)
		TEE_Panic(0);

	if ((info.handleFlags & TEE_HANDLE_FLAG_PERSISTENT) != 0)
		TEE_Panic(0);

	res = utee_cryp_obj_reset((uint32_t)object);
	if (res != TEE_SUCCESS)
		TEE_Panic(0);
}

TEE_Result TEE_PopulateTransientObject(TEE_ObjectHandle object,
				       TEE_Attribute *attrs,
				       uint32_t attrCount)
{
	TEE_Result res;
	TEE_ObjectInfo info;

	res = utee_cryp_obj_get_info((uint32_t)object, &info);
	if (res != TEE_SUCCESS)
		TEE_Panic(0);

	/* Must be a transient object */
	if ((info.handleFlags & TEE_HANDLE_FLAG_PERSISTENT) != 0)
		TEE_Panic(0);

	/* Must not be initialized already */
	if ((info.handleFlags & TEE_HANDLE_FLAG_INITIALIZED) != 0)
		TEE_Panic(0);

	res = utee_cryp_obj_populate((uint32_t)object, attrs, attrCount);
	if (res != TEE_SUCCESS && res != TEE_ERROR_BAD_PARAMETERS)
		TEE_Panic(res);
	return res;
}

void TEE_InitRefAttribute(TEE_Attribute *attr, uint32_t attributeID,
			  void *buffer, size_t length)
{
	if (attr == NULL)
		TEE_Panic(0);
	if ((attributeID & TEE_ATTR_BIT_VALUE) != 0)
		TEE_Panic(0);
	attr->attributeID = attributeID;
	attr->content.ref.buffer = buffer;
	attr->content.ref.length = length;
}

void TEE_InitValueAttribute(TEE_Attribute *attr, uint32_t attributeID,
			    uint32_t a, uint32_t b)
{
	if (attr == NULL)
		TEE_Panic(0);
	if ((attributeID & TEE_ATTR_BIT_VALUE) == 0)
		TEE_Panic(0);
	attr->attributeID = attributeID;
	attr->content.value.a = a;
	attr->content.value.b = b;
}

void TEE_CopyObjectAttributes(TEE_ObjectHandle destObject,
			      TEE_ObjectHandle srcObject)
{
	TEE_Result res;
	TEE_ObjectInfo dst_info;
	TEE_ObjectInfo src_info;

	res = utee_cryp_obj_get_info((uint32_t)destObject, &dst_info);
	if (res != TEE_SUCCESS)
		TEE_Panic(0);

	res = utee_cryp_obj_get_info((uint32_t)srcObject, &src_info);
	if (res != TEE_SUCCESS)
		TEE_Panic(0);

	if ((src_info.handleFlags & TEE_HANDLE_FLAG_INITIALIZED) == 0)
		TEE_Panic(0);
	if ((dst_info.handleFlags & TEE_HANDLE_FLAG_PERSISTENT) != 0)
		TEE_Panic(0);
	if ((dst_info.handleFlags & TEE_HANDLE_FLAG_INITIALIZED) != 0)
		TEE_Panic(0);

	res = utee_cryp_obj_copy((uint32_t)destObject, (uint32_t)srcObject);
	if (res != TEE_SUCCESS)
		TEE_Panic(0);
}

TEE_Result TEE_GenerateKey(TEE_ObjectHandle object, uint32_t keySize,
			   TEE_Attribute *params, uint32_t paramCount)
{
	TEE_Result res;

	res = utee_cryp_obj_generate_key((uint32_t)object, keySize,
					 params, paramCount);

	if (res != TEE_SUCCESS)
		TEE_Panic(0);

	return res;
}

/* Data and Key Storage API  - Persistent Object Functions */

TEE_Result TEE_OpenPersistentObject(uint32_t storageID, void *objectID,
				    size_t objectIDLen, uint32_t flags,
				    TEE_ObjectHandle *object)
{
	if (storageID != TEE_STORAGE_PRIVATE)
		return TEE_ERROR_ITEM_NOT_FOUND;

	if (objectID == NULL)
		return TEE_ERROR_ITEM_NOT_FOUND;

	if (objectIDLen > TEE_OBJECT_ID_MAX_LEN)
		TEE_Panic(0);

	if (object == NULL)
		return TEE_ERROR_BAD_PARAMETERS;

	return utee_storage_obj_open(storageID, objectID, objectIDLen, flags,
				     object);
}

TEE_Result TEE_CreatePersistentObject(uint32_t storageID, void *objectID,
				      size_t objectIDLen, uint32_t flags,
				      TEE_ObjectHandle attributes,
				      const void *initialData,
				      size_t initialDataLen,
				      TEE_ObjectHandle *object)
{
	if (storageID != TEE_STORAGE_PRIVATE)
		return TEE_ERROR_ITEM_NOT_FOUND;

	if (objectID == NULL)
		return TEE_ERROR_ITEM_NOT_FOUND;

	if (objectIDLen > TEE_OBJECT_ID_MAX_LEN)
		TEE_Panic(0);

	if (object == NULL)
		return TEE_ERROR_BAD_PARAMETERS;

	return utee_storage_obj_create(storageID, objectID, objectIDLen, flags,
				       attributes, initialData, initialDataLen,
				       object);
}

void TEE_CloseAndDeletePersistentObject(TEE_ObjectHandle object)
{
	TEE_Result res;

	if (object == TEE_HANDLE_NULL)
		return;

	res = utee_storage_obj_del(object);

	if (res != TEE_SUCCESS)
		TEE_Panic(0);
}

TEE_Result TEE_RenamePersistentObject(TEE_ObjectHandle object,
				      const void *newObjectID,
				      size_t newObjectIDLen)
{
	TEE_Result res;

	if (object == TEE_HANDLE_NULL)
		return TEE_ERROR_ITEM_NOT_FOUND;

	if (newObjectID == NULL)
		return TEE_ERROR_BAD_PARAMETERS;

	if (newObjectIDLen > TEE_OBJECT_ID_MAX_LEN)
		TEE_Panic(0);

	res = utee_storage_obj_rename(object, newObjectID, newObjectIDLen);

	if (res != TEE_SUCCESS && res != TEE_ERROR_ACCESS_CONFLICT)
		TEE_Panic(0);

	return res;
}

TEE_Result TEE_AllocatePersistentObjectEnumerator(TEE_ObjectEnumHandle *
						  objectEnumerator)
{
	TEE_Result res;

	if (objectEnumerator == NULL)
		return TEE_ERROR_BAD_PARAMETERS;

	res = utee_storage_alloc_enum(objectEnumerator);

	if (res != TEE_SUCCESS)
		*objectEnumerator = TEE_HANDLE_NULL;

	return res;
}

void TEE_FreePersistentObjectEnumerator(TEE_ObjectEnumHandle objectEnumerator)
{
	TEE_Result res;

	if (objectEnumerator == TEE_HANDLE_NULL)
		return;

	res = utee_storage_free_enum(objectEnumerator);

	if (res != TEE_SUCCESS)
		TEE_Panic(0);
}

void TEE_ResetPersistentObjectEnumerator(TEE_ObjectEnumHandle objectEnumerator)
{
	TEE_Result res;

	if (objectEnumerator == TEE_HANDLE_NULL)
		return;

	res = utee_storage_reset_enum(objectEnumerator);

	if (res != TEE_SUCCESS)
		TEE_Panic(0);
}

TEE_Result TEE_StartPersistentObjectEnumerator(TEE_ObjectEnumHandle
					       objectEnumerator,
					       uint32_t storageID)
{
	TEE_Result res;

	if (storageID != TEE_STORAGE_PRIVATE)
		return TEE_ERROR_ITEM_NOT_FOUND;

	res = utee_storage_start_enum(objectEnumerator, storageID);

	if (res != TEE_SUCCESS && res != TEE_ERROR_ITEM_NOT_FOUND)
		TEE_Panic(0);

	return res;
}

TEE_Result TEE_GetNextPersistentObject(TEE_ObjectEnumHandle objectEnumerator,
				       TEE_ObjectInfo *objectInfo,
				       void *objectID, size_t *objectIDLen)
{
	TEE_Result res;

	res =
	    utee_storage_next_enum(objectEnumerator, objectInfo, objectID,
				   objectIDLen);

	if (res != TEE_SUCCESS && res != TEE_ERROR_ITEM_NOT_FOUND)
		TEE_Panic(0);

	return res;
}

/* Data and Key Storage API  - Data Stream Access Functions */

TEE_Result TEE_ReadObjectData(TEE_ObjectHandle object, void *buffer,
			      size_t size, uint32_t *count)
{
	TEE_Result res;

	if (object == TEE_HANDLE_NULL)
		TEE_Panic(0);

	res = utee_storage_obj_read(object, buffer, size, count);

	if (res != TEE_SUCCESS)
		TEE_Panic(0);

	return res;
}

TEE_Result TEE_WriteObjectData(TEE_ObjectHandle object, void *buffer,
			       size_t size)
{
	TEE_Result res;

	if (object == TEE_HANDLE_NULL)
		TEE_Panic(0);

	res = utee_storage_obj_write(object, buffer, size);

	if (res != TEE_SUCCESS && res != TEE_ERROR_STORAGE_NO_SPACE)
		TEE_Panic(0);

	return res;
}

TEE_Result TEE_TruncateObjectData(TEE_ObjectHandle object, uint32_t size)
{
	TEE_Result res;

	if (object == TEE_HANDLE_NULL)
		TEE_Panic(0);

	res = utee_storage_obj_trunc(object, size);

	if (res != TEE_SUCCESS && res != TEE_ERROR_STORAGE_NO_SPACE)
		TEE_Panic(0);

	return res;
}

TEE_Result TEE_SeekObjectData(TEE_ObjectHandle object, int32_t offset,
			      TEE_Whence whence)
{
	TEE_Result res;
	TEE_ObjectInfo info;

	if (object == TEE_HANDLE_NULL)
		TEE_Panic(0);

	res = utee_cryp_obj_get_info((uint32_t)object, &info);
	if (res != TEE_SUCCESS)
		TEE_Panic(0);

	switch (whence) {
	case TEE_DATA_SEEK_SET:
		if (offset > 0 && (uint32_t)offset > TEE_DATA_MAX_POSITION)
			return TEE_ERROR_OVERFLOW;
		break;
	case TEE_DATA_SEEK_CUR:
		if (offset > 0 &&
		    ((uint32_t)offset + info.dataPosition >
		     TEE_DATA_MAX_POSITION ||
		     (uint32_t)offset + info.dataPosition <
		     info.dataPosition))
			return TEE_ERROR_OVERFLOW;
		break;
	case TEE_DATA_SEEK_END:
		if (offset > 0 &&
		    ((uint32_t)offset + info.dataSize > TEE_DATA_MAX_POSITION ||
		     (uint32_t)offset + info.dataSize < info.dataSize))
			return TEE_ERROR_OVERFLOW;
		break;
	default:
		TEE_Panic(0);
	}

	res = utee_storage_obj_seek(object, offset, whence);

	if (res != TEE_SUCCESS && res != TEE_ERROR_OVERFLOW)
		TEE_Panic(0);

	return res;
}
