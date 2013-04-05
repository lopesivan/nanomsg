/*
    Copyright (c) 2012 250bpm s.r.o.

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom
    the Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
    IN THE SOFTWARE.
*/

#include "thread.h"
#include "err.h"

#ifdef NN_HAVE_WINDOWS

static unsigned int __stdcall nn_thread_main_routine (void *arg)
{
    struct nn_thread *self;

    self = (struct nn_thread*) arg;
    self->routine (self->arg);
    return 0;
}

void nn_thread_init (struct nn_thread *self,
    nn_thread_routine *routine, void *arg)
{
    self->routine = routine;
    self->arg = arg;
    self->handle = (HANDLE) _beginthreadex (NULL, 0,
        nn_thread_main_routine, (void*) self, 0 , NULL);
    win_assert (self->handle != NULL);
}

void nn_thread_term (struct nn_thread *self)
{
    DWORD rc;
    BOOL brc;

    rc = WaitForSingleObject (self->handle, INFINITE);
    win_assert (rc != WAIT_FAILED);
    brc = CloseHandle (self->handle);
    win_assert (brc != 0);
}

#else

#include <signal.h>

static void *nn_thread_main_routine (void *arg)
{
    int rc;
    sigset_t sigset;
    struct nn_thread *self;

    self = (struct nn_thread*) arg;
   
    /*  No signals should be processed by this thread. The library doesn't
        use signals and thus all the signals should be delivered to application
        threads, not to worker threads. */
    rc = sigfillset (&sigset);
    errno_assert (rc == 0);
    rc = pthread_sigmask (SIG_BLOCK, &sigset, NULL);
    errnum_assert (rc == 0, rc);

    /*  Run the thread routine. */
    self->routine (self->arg);
    return NULL;
}

void nn_thread_init (struct nn_thread *self,
    nn_thread_routine *routine, void *arg)
{
    int rc;

    self->routine = routine;
    self->arg = arg;
    rc = pthread_create (&self->handle, NULL, nn_thread_main_routine,
        (void*) self);
    errnum_assert (rc == 0, rc);
}

void nn_thread_term (struct nn_thread *self)
{
    int rc;

    rc = pthread_join (self->handle, NULL);
    errnum_assert (rc == 0, rc);
}

#endif

