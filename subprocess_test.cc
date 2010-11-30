#include "subprocess.h"

#include "test.h"

TEST(Subprocess, Ls) {
  Subprocess ls;
  EXPECT_TRUE(ls.Start("ls /"));

  // Pretend we discovered that stdout was ready for writing.
  ls.OnFDReady(ls.stdout_.fd_);

  string err;
  EXPECT_TRUE(ls.Finish(&err));
  ASSERT_EQ("", err);
  EXPECT_NE("", ls.stdout_.buf_);
  EXPECT_EQ("", ls.stderr_.buf_);
}

TEST(Subprocess, BadCommand) {
  Subprocess subproc;
  EXPECT_TRUE(subproc.Start("ninja_no_such_command"));

  // Pretend we discovered that stderr was ready for writing.
  subproc.OnFDReady(subproc.stderr_.fd_);

  string err;
  EXPECT_FALSE(subproc.Finish(&err));
  EXPECT_NE("", err);
  EXPECT_EQ("", subproc.stdout_.buf_);
  EXPECT_NE("", subproc.stderr_.buf_);
}

TEST(SubprocessSet, Single) {
  SubprocessSet subprocs;
  Subprocess* ls = new Subprocess;
  EXPECT_TRUE(ls->Start("ls /"));
  subprocs.Add(ls);

  string err;
  while (!ls->done()) {
    subprocs.DoWork(&err);
    ASSERT_EQ("", err);
  }
  ASSERT_NE("", ls->stdout_.buf_);

  ASSERT_EQ(1, subprocs.finished_.size());
}

TEST(SubprocessSet, Multi) {
  SubprocessSet subprocs;
  Subprocess* processes[3];
  const char* kCommands[3] = {
    "ls /",
    "whoami",
    "pwd",
  };

  for (int i = 0; i < 3; ++i) {
    processes[i] = new Subprocess;
    EXPECT_TRUE(processes[i]->Start(kCommands[i]));
    subprocs.Add(processes[i]);
  }

  ASSERT_EQ(3, subprocs.running_.size());
  for (int i = 0; i < 3; ++i) {
    ASSERT_FALSE(processes[i]->done());
    ASSERT_EQ("", processes[i]->stdout_.buf_);
    ASSERT_EQ("", processes[i]->stderr_.buf_);
  }

  string err;
  while (!processes[0]->done() || !processes[1]->done() ||
         !processes[2]->done()) {
    ASSERT_GT(subprocs.running_.size(), 0);
    subprocs.DoWork(&err);
    ASSERT_EQ("", err);
  }

  ASSERT_EQ(0, subprocs.running_.size());
  ASSERT_EQ(3, subprocs.finished_.size());

  for (int i = 0; i < 3; ++i) {
    ASSERT_NE("", processes[i]->stdout_.buf_);
    ASSERT_EQ("", processes[i]->stderr_.buf_);
    delete processes[i];
  }
}

