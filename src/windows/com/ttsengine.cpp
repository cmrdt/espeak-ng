/*
 * Copyright (C) 2016 Reece H. Dunn
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see: <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <windows.h>
#include <sapiddk.h>
#include <sperror.h>

#include <new>

extern "C" ULONG ObjectCount;

struct TtsEngine
	: public ISpObjectWithToken
	, public ISpTTSEngine
{
	TtsEngine();
	~TtsEngine();

	// IUnknown

	ULONG __stdcall AddRef();
	ULONG __stdcall Release();

	HRESULT __stdcall QueryInterface(REFIID iid, void **object);

	// ISpObjectWithToken

	HRESULT __stdcall GetObjectToken(ISpObjectToken **token);
	HRESULT __stdcall SetObjectToken(ISpObjectToken *token);

	// ISpTTSEngine

	HRESULT __stdcall
	Speak(DWORD flags,
	      REFGUID formatId,
	      const WAVEFORMATEX *format,
	      const SPVTEXTFRAG *textFragList,
	      ISpTTSEngineSite *site);

	HRESULT __stdcall
	GetOutputFormat(const GUID *targetFormatId,
	                const WAVEFORMATEX *targetFormat,
	                GUID *formatId,
	                WAVEFORMATEX **format);
private:
	ULONG refCount;
	ISpObjectToken *objectToken;
};

TtsEngine::TtsEngine()
	: refCount(1)
	, objectToken(NULL)
{
	InterlockedIncrement(&ObjectCount);
}

TtsEngine::~TtsEngine()
{
	InterlockedDecrement(&ObjectCount);
	if (objectToken)
		objectToken->Release();
}

ULONG __stdcall TtsEngine::AddRef()
{
	return InterlockedIncrement(&refCount);
}

ULONG __stdcall TtsEngine::Release()
{
	ULONG ret = InterlockedDecrement(&refCount);
	if (ret == 0)
		delete this;
	return ret;
}

HRESULT __stdcall TtsEngine::QueryInterface(REFIID iid, void **object)
{
	*object = NULL;
	if (IsEqualIID(iid, IID_IUnknown) || IsEqualIID(iid, IID_ISpTTSEngine))
		*object = (ISpTTSEngine *)this;
	else if (IsEqualIID(iid, IID_ISpObjectWithToken))
		*object = (ISpObjectWithToken *)this;
	else
		return E_NOINTERFACE;

	this->AddRef();
	return S_OK;
}

HRESULT __stdcall TtsEngine::GetObjectToken(ISpObjectToken **token)
{
	if (!token)
		return E_POINTER;

	*token = objectToken;
	if (objectToken) {
		objectToken->AddRef();
		return S_OK;
	}
	return S_FALSE;
}

HRESULT __stdcall TtsEngine::SetObjectToken(ISpObjectToken *token)
{
	if (!token)
		return E_INVALIDARG;

	if (objectToken)
		return SPERR_ALREADY_INITIALIZED;

	objectToken = token;
	objectToken->AddRef();

	return S_OK;
}

HRESULT __stdcall
TtsEngine::Speak(DWORD flags,
                 REFGUID formatId,
                 const WAVEFORMATEX *format,
                 const SPVTEXTFRAG *textFragList,
                 ISpTTSEngineSite *site)
{
	return E_NOTIMPL;
}

HRESULT __stdcall
TtsEngine::GetOutputFormat(const GUID *targetFormatId,
                           const WAVEFORMATEX *targetFormat,
                           GUID *formatId,
                           WAVEFORMATEX **format)
{
	*format = (WAVEFORMATEX *)CoTaskMemAlloc(sizeof(WAVEFORMATEX));
	if (!*format)
		return E_OUTOFMEMORY;
	(*format)->wFormatTag = WAVE_FORMAT_PCM;
	(*format)->nChannels = 1;
	(*format)->nBlockAlign = 2;
	(*format)->nSamplesPerSec = 22050;
	(*format)->wBitsPerSample = 16;
	(*format)->nAvgBytesPerSec = (*format)->nAvgBytesPerSec * (*format)->nBlockAlign;
	(*format)->cbSize = 0;
	*formatId = SPDFID_WaveFormatEx;
	return S_OK;
}

extern "C" HRESULT __stdcall TtsEngine_CreateInstance(IClassFactory *iface, IUnknown *outer, REFIID iid, void **object)
{
	if (outer != NULL)
		return CLASS_E_NOAGGREGATION;

	TtsEngine *engine = new (std::nothrow) TtsEngine();
	if (!engine)
		return E_OUTOFMEMORY;

	HRESULT ret = engine->QueryInterface(iid, object);
	engine->Release();
	return ret;
}