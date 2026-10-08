/* Test parts: a test program started as "<program> part K/N" runs only
   groups K, K + N, K + 2N... of its list, so test.bat can run N copies at
   once and the slowest program stops setting the length of the whole run.
   Each GROUP line is one group, counted in the order the program reaches
   them, nested lists included, so every copy counts the same. A list
   that holds GROUP lines is called bare, never inside a GROUP itself:
   the copies that skipped it would count fewer groups and lose tests.
   With no argument every group runs. */

global_variable u32 TestPart;
global_variable u32 TestParts = 1;
global_variable u32 TestGroupIndex;

inline bool32
InTestPart()
{
    bool32 Result = (TestGroupIndex % TestParts) == TestPart;
    TestGroupIndex++;
    return Result;
}

#define GROUP(Call) if (InTestPart()) { Call; }
// NOTE(zoubir): names the test as it starts, so a crash shows which one
#define RUN(Test) printf("%s\n", #Test); Test()

// NOTE(zoubir): reads "part K/N" from the command line; anything else
// leaves every group on
internal void
ReadTestPart(int ArgCount, char **Args)
{
    for(int Arg = 1; Arg + 1 < ArgCount; Arg++)
    {
        unsigned Part = 0, Parts = 0;
        if (strcmp(Args[Arg], "part") == 0 &&
            sscanf(Args[Arg + 1], "%u/%u", &Part, &Parts) == 2 && Parts > 0 && Part < Parts)
        {
            TestPart = Part;
            TestParts = Parts;
        }
    }
}
