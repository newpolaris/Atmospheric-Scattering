// +----------------------------------------------------------------------
// | Project : ray.
// | All rights reserved.
// +----------------------------------------------------------------------
// | Copyright (c) 2013-2015.
// +----------------------------------------------------------------------
// | * Redistribution and use of this software in source and binary forms,
// |   with or without modification, are permitted provided that the following
// |   conditions are met:
// |
// | * Redistributions of source code must retain the above
// |   copyright notice, this list of conditions and the
// |   following disclaimer.
// |
// | * Redistributions in binary form must reproduce the above
// |   copyright notice, this list of conditions and the
// |   following disclaimer in the documentation and/or other
// |   materials provided with the distribution.
// |
// | * Neither the name of the ray team, nor the names of its
// |   contributors may be used to endorse or promote products
// |   derived from this software without specific prior
// |   written permission of the ray team.
// |
// | THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// | "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// | LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// | A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// | OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// | SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// | LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// | DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// | THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// | (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// | OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// +----------------------------------------------------------------------
#pragma once

#include <tools/RttiMacros.h>
#include <memory>
#include <string>
#include <cassert>

namespace rtti
{
	typedef std::shared_ptr<class Interface> InterfacePtr;

    class Rtti final
    {
	public:
		typedef Interface*(*RttiConstruct)();
	public:
		Rtti(const std::string& name, RttiConstruct creator, const Rtti* parent) noexcept;
		~Rtti() noexcept;

		const Rtti* getParent() const noexcept;

		bool isDerivedFrom(const Rtti* other) const;
		bool isDerivedFrom(const Rtti& other) const;
		bool isDerivedFrom(const std::string& name) const;

	private:

		std::string _name;
		const Rtti* _parent;
		RttiConstruct _construct;
    };

	class Interface : public std::enable_shared_from_this<Interface>
	{
		__DeclareClass(Interface)
	public:

		Interface() noexcept;
		virtual ~Interface() noexcept;

		bool isA(const Rtti* rtti) const noexcept;
		bool isA(const Rtti& rtti) const noexcept;
		bool isA(const std::string& rttiName) const noexcept;

		template<typename T>
		bool isA() const noexcept
		{
			return this->isA(T::getRtti());
		}

		template<typename T>
		std::shared_ptr<T> downcast_pointer() noexcept
		{
			assert(this->isA<T>());
			return std::dynamic_pointer_cast<T>(this->shared_from_this());
		}

    };
}
