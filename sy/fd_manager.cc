#include "fd_manager.h"
#include "hook.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

namespace sy {

FdCtx::FdCtx(int fd)
    :m_isInit(false)
    ,m_isSocket(false)
    ,m_sysNonblock(false)
    ,m_userNonblock(false)
    ,m_isClosed(false)
    ,m_fd(fd)
    ,m_recvTimeout(-1)
    ,m_sendTimeout(-1) {
    init();
}

FdCtx::~FdCtx() {
}

// 初始化
bool FdCtx::init() {
    if(m_isInit) { // 初始化过了
        return true;
    }
    m_recvTimeout = -1;
    m_sendTimeout = -1;

    struct stat fd_stat;
    // return 0 : 成功取出
    // return -1 : 取出失败，关闭了
    if (-1 == fstat(m_fd, &fd_stat)) {
        m_isInit = false;
        m_isSocket = false;
    } else {
        m_isInit = true;
        // 判断文件是否为socket
        m_isSocket = S_ISSOCK(fd_stat.st_mode);
    }
    
    // 是socket则设置为非阻塞模式
    if (m_isSocket) {
        int flag = fcntl_f(m_fd, F_GETFL, 0);
        if (!(flag & O_NONBLOCK)) {
            fcntl_f(m_fd, F_SETFL, flag | O_NONBLOCK);
        }
        m_sysNonblock = true;
    } else {
        m_sysNonblock = false;
    }

    m_userNonblock = false;
    m_isClosed = false;
    return m_isInit;
}

void FdCtx::setTimeout(int type, uint64_t v) {
    if(type == SO_RCVTIMEO) {
        m_recvTimeout = v;
    } else {
        m_sendTimeout = v;
    }
}

uint64_t FdCtx::getTimeout(int type) {
    if(type == SO_RCVTIMEO) {
        return m_recvTimeout;
    } else {
        return m_sendTimeout;
    }
}

FdManager::FdManager() {
    m_datas.resize(64);
}

FdCtx::ptr FdManager::get(int fd, bool auto_create) {
    if(fd == -1) {
        return nullptr;
    }
    // 集合中没有，并且不自动创建，返回nullptr
    RWMUtexType::ReadLock lock(m_mutex);
    if ((int)m_datas.size() <= fd) {
        if (auto_create == false) {
            return nullptr;
        }
    } else {
        // 找到了直接返回
        if (m_datas[fd] || !auto_create) {
            return m_datas[fd];
        }
    }
    lock.unlock();
	
    RWMUtexType::WriteLock locK2(m_mutex);
    // 创建新的FdCtx
    FdCtx::ptr ctx(new FdCtx(fd));
    // fd比集合下标大，扩充
    if (fd >= (int)m_datas.size()) {
        m_datas.resize(fd * 1.5);
    }
    // 放入集合中
    m_datas[fd] = ctx;
    return ctx;
}

void FdManager::del(int fd) {
    RWMutexType::WriteLock lock(m_mutex);
    if((int)m_datas.size() <= fd) {
        return;
    }
    m_datas[fd].reset();
}

}
