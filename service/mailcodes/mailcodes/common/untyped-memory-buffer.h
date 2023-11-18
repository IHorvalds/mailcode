#pragma once
#include <span>

class UntypedMemoryBuffer
{
public:

  explicit UntypedMemoryBuffer(void* data, size_t size)
    : m_pData(data),
      m_uSize(size)
  { }

  ~UntypedMemoryBuffer()
  {
    if (m_pData)
    {
      free(m_pData);
    }
  }

  bool append(void* newData, size_t extraSize)
  {
    void* p_OldData = m_pData;

    m_pData = realloc(m_pData, m_uSize + extraSize);

    if (!m_pData)
    {
      // clean up old data
      free(p_OldData);
      p_OldData = nullptr;
      return false;
    }

    memcpy(static_cast<char*>(m_pData) + m_uSize, newData, extraSize);
    m_uSize += extraSize;

    return true;
  }

  template <typename T>
  std::span<T> getData()
  {
    return std::span<T>((T*)m_pData, m_uSize);
  }

private:
  void* m_pData;
  size_t m_uSize;
};