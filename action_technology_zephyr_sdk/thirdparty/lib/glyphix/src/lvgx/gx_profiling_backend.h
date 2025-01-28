#pragma once

#include "gx_file.h"
#include "gx_profiling.h"

#ifdef GX_ENABLE_PROFILING
class ProfilingBackend : public gx::prof::Backend {
public:
  ProfilingBackend() : m_file("/data/prof.pp") {
    setup();
  }
  ~ProfilingBackend() {}
  virtual void startLogging() {
    if (m_file.open(gx::File::WriteOnly))
      m_file.write(headFrame().data(), headFrame().size());
  }
  virtual void stopLogging() { m_file.close(); }
  virtual void postFrame(const gx::Vector<char> &frame) {
    if (m_file.isOpened())
      m_file.write(frame.data(), frame.size());
  }

private:
  gx::File m_file;
};
#else
struct ProfilingBackend {
  ProfilingBackend() {}
};
#endif
