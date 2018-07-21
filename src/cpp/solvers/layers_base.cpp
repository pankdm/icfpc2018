#include "layers_base.h"

SolverLayersBase::SolverLayersBase(const Matrix& m) : matrix(m)
{
    state.Init(m.GetR(), Trace());
}

void SolverLayersBase::MoveToCoordinate(State::BotState& bs, int x, int z)
{
    Command c(Command::SMove);
    for (; abs(x - bs.c.x) > 5; )
    {
        c.cd1 = {max(-15, min(x - bs.c.x, 15)), 0, 0};
        AddCommand(c);
    }
    for (; abs(z - bs.c.z) > 5; )
    {
        c.cd1 = {0, 0, max(-15, min(z - bs.c.z, 15))};
        AddCommand(c);
    }
    if (bs.c.x == x)
    {
        if (bs.c.z == z)
        {
            // Already here
        }
        else
        {
            c.cd1 = {0, 0, z - bs.c.z};
            AddCommand(c);
        }
    }
    else
    {
        if (bs.c.z == z)
        {
            c.cd1 = {x - bs.c.x, 0, 0};
            AddCommand(c);
        }
        else
        {
            c.type = Command::LMove;
            c.cd1 = {x - bs.c.x, 0, 0};
            c.cd2 = {0, 0, z - bs.c.z};
            AddCommand(c);
        }
    }
}

void SolverLayersBase::MoveToCoordinate(State::BotState& bs, int x, int y, int z)
{
    Command c(Command::SMove);
    c.cd1 = {0, 0, 0};
    if ((x == 0) && (y == 0) && (z == 0))
    {
        MoveToCoordinate(bs, x, z);
        for (; bs.c.y > 0; )
        {
            c.cd1.dy = max(-15, -bs.c.y);
            AddCommand(c);
        }
    }
    else
    {
        for (; bs.c.y != y; )
        {
            c.cd1.dy = max(-15, min(y - bs.c.y, 15));
            AddCommand(c);
        }
        MoveToCoordinate(bs, x, z);
    }
}

void SolverLayersBase::SolveInit()
{
    AddCommand(Command(Command::Flip));
}

void SolverLayersBase::SolveZ1_GetRZ(int x, int y, int& z0, int& z1)
{
    int r = matrix.GetR();
    z0 = r, z1 = -1;
    for (int z = 0; z < r; ++z)
    {
        if (matrix.Get(x, y, z) && !state.matrix.Get(x, y, z))
        {
            z0 = min(z0, z);
            z1 = max(z1, z);
        }
    }
}

void SolverLayersBase::SolveZ1_Fill(State::BotState& bs, int x, int y, bool direction)
{
    for (;;)
    {
        for (int dz = -1; dz <= 1; ++dz)
        {
            Coordinate c {x, y, bs.c.z + dz};
            if (matrix.IsInside(c) && matrix.Get(c))
            {
                Command m(Command::Fill);
                m.cd1 = { 0, -1, dz};
                AddCommand(m);
            }
        }
        int z0, z1;
        SolveZ1_GetRZ(x, y, z0, z1);
        if (z1 < 0) return; // Nothing to do
        int nextz = (z0 == z1) ? z0 : direction ? z0 + 1 : z1 - 1;
        MoveToCoordinate(bs, x, y + 1, nextz);
    }    
}

void SolverLayersBase::SolveZ1(int x, int y)
{
    int z0, z1;
    SolveZ1_GetRZ(x, y, z0, z1);
    if (z1 < 0) return; // Nothing to do

    State::BotState& bs = state.all_bots[0];
    bool zdirection = (bs.c.z <= (z0 + z1) / 2);
    int zstart = (z0 == z1) ? z0 : zdirection ? z0 + 1 : z1 - 1;
    MoveToCoordinate(bs, x, y + 1, zstart);
    SolveZ1_Fill(bs, x, y, zdirection);
}

void SolverLayersBase::SolveZ3_GetRZ(int x, int y, int& z0, int& z1)
{
    int r = matrix.GetR();
    z0 = r, z1 = -1;
    for (int ix = max(x - 1, 0); ix <= min(x + 1, r - 1); ++ix)
    {
        for (int z = 0; z < r; ++z)
        {
            if (matrix.Get(ix, y, z) && !state.matrix.Get(ix, y, z))
            {
                z0 = min(z0, z);
                z1 = max(z1, z);
            }
        }
    }
}

void SolverLayersBase::SolveZ3_Fill(State::BotState& bs, int x, int y, bool direction)
{
    for (;;)
    {
        for (int dx = -1; dx <= 1; ++dx)
        {
            Coordinate c {x + dx, y, bs.c.z};
            if (matrix.IsInside(c) && matrix.Get(c))
            {
                Command m(Command::Fill);
                m.cd1 = { dx, -1, 0 };
                AddCommand(m);
            }
        }
        int z0, z1;
        SolveZ3_GetRZ(x, y, z0, z1);
        if (z1 < 0) return; // Nothing to do
        int nextz = direction ? z0 : z1;
        MoveToCoordinate(bs, x, y + 1, nextz);
    }
}

void SolverLayersBase::SolveZ3(int x, int y)
{
    int z0, z1;
    SolveZ3_GetRZ(x, y, z0, z1);
    if (z1 < 0) return; // Nothing to do

    State::BotState& bs = state.all_bots[0];
    bool zdirection = (bs.c.z <= (z0 + z1) / 2);
    int zstart = zdirection ? z0 : z1;
    MoveToCoordinate(bs, x, y + 1, zstart);
    SolveZ3_Fill(bs, x, y, zdirection);
}

void SolverLayersBase::SolveLayer(int y)
{
    int r = matrix.GetR();
    // Get box
    int x0 = r, x1 = -1, z0 = r, z1 = -1;
    for (int x = 0; x < r; ++x)
    {
        for (int z = 0; z < r; ++z)
        {
            if (matrix.Get(x, y, z))
            {
                x0 = min(x0, x);
                x1 = max(x1, x);
                z0 = min(z0, z);
                z1 = max(z1, z);
            }
        }
    }
    if (x1 < 0) return; // Nothing to do
    Coordinate c = state.all_bots[0].c;
    if (c.x <= (x0 + x1) / 2)
    {
        for (int x = x0; x <= x1;)
        {
            if (x < x1)
            {
                SolveZ3(x + 1, y);
                x += 3;
            }
            else
            {
                SolveZ1(x, y);
                x += 1;
            }
        }
    }
    else
    {
        for (int x = x1; x >= x0;)
        {
            if (x > x0)
            {
                SolveZ3(x - 1, y);
                x -= 3;
            }
            else
            {
                SolveZ1(x, y);
                x -= 1;
            }
        }
    }
}

void SolverLayersBase::SolveFinalize()
{
   AddCommand(Command(Command::Flip));
   MoveToCoordinate(state.all_bots[0], 0, 0, 0);
   AddCommand(Command(Command::Halt));
}

void SolverLayersBase::Solve(Trace& output)
{
    SolveInit();
    for (int i = 0; i < matrix.GetR() - 1; ++i)
    {
        SolveLayer(i);
    }
    SolveFinalize();
    output = state.trace;
}

uint64_t SolverLayersBase::Solve(const Matrix& m, Trace& output)
{
    SolverLayersBase solver(m);
    solver.Solve(output);
    return solver.state.IsCorrectFinal() ? solver.state.energy : 0;
}
