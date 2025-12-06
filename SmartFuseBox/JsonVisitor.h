#pragma once


class JsonVisitor
{
public:
	virtual void formatStatusJson(char* buffer, size_t size) = 0;
	virtual ~JsonVisitor() = default;
};