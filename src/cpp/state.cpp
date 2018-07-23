#include "command_line.h"
#include "state.h"
#include "grounder.h"
#include "constants.h"
#include "region.h"

void State::Init(const Matrix& source, const Trace& _trace)
{
    correct = true;
    energy = 0;
    harmonics = 0;
    matrix = source;
    backMatrix = source;
    all_bots.resize(0);
    all_bots.resize(TaskConsts::N_BOTS);
    for (unsigned i = 0; i < TaskConsts::N_BOTS; ++i)
    {
        all_bots[i].bid = i;
        if (i) {
            all_bots[0].seeds.push_back(i);
        }
    }
    all_bots[0].c = {0, 0, 0};
    active_bots.push_back(0);
    trace = _trace;
    trace_pos = 0;
}

bool State::IsCorrectFinal() const
{
    return correct && (harmonics == 0) && (active_bots.size() == 0) && (trace_pos == trace.size());
}


void State::Fulfill() {
  for (auto v: toAdd) {
    backMatrix.Fill(v);
  }
  for (auto v: toDelete) {
    backMatrix.Erase(v);
  }
  toAdd.clear();
  toDelete.clear();
}

bool State::IsGrounded() {
  if (toDelete.empty()) {
    bool result = Grounder::IsDeltaGrounded(backMatrix, toAdd);
    Fulfill();
    return result;
  } else {
    Fulfill();
    return matrix.IsGrounded();
  }
}

void State::Step()
{
    vector<unsigned> bots = active_bots;
    // for (auto bid : active_bots) {
    //   cout << " bot " << bid << " at " << all_bots[bid].c << endl;
    // }

    assert(trace_pos + bots.size() <= trace.size());
    energy += (harmonics ? 30 : 3) * matrix.GetVolume();
    int hc = 0;
    energy += 20 * bots.size();
    InterfereCheck ic;
    map<Region, vector<Coordinate>> fills;
    map<Region, vector<Coordinate>> erases;
    for (unsigned bid : bots)
    {
        BotState& bs = all_bots[bid];
        ic.AddCoordinate(bs.c);
        Command c = trace.commands[trace_pos++];
        if (c.type == Command::Halt)
        {
            correct = correct && (bs.c.x == 0) && (bs.c.y == 0) && (bs.c.z == 0) && (bots.size() == 1) && (!harmonics) && (trace_pos == trace.size());
            assert(correct);
            active_bots.clear();
        }
        else if (c.type == Command::Wait)
        {
        }
        else if (c.type == Command::Flip)
        {
            hc++;
        }
        else if (c.type == Command::SMove)
        {
            correct = correct && c.cd1.IsLongLinearCoordinateDifferences();
            assert(correct);
            correct = correct && MoveBot(bs, ic, c.cd1);
            assert(correct);
        }
        else if (c.type == Command::LMove)
        {
            correct = correct && c.cd1.IsShortLinearCoordinateDifferences() && MoveBot(bs, ic, c.cd1);
            correct = correct && c.cd2.IsShortLinearCoordinateDifferences() && MoveBot(bs, ic, c.cd2);
            assert(correct);
            energy += 4;
        }
        else if (c.type == Command::Fission)
        {
            Coordinate fc = bs.c + c.cd1;
            correct = correct && matrix.IsInside(fc) && !matrix.Get(fc) && (c.m + 1 <= bs.seeds.size());
            assert(correct);
            active_bots.push_back(bs.seeds[0]);
            sort(active_bots.begin(), active_bots.end());
            BotState& jbs = all_bots[bs.seeds[0]];
            jbs.c = fc;
            jbs.seeds = vector<unsigned> {bs.seeds.begin() + 1, bs.seeds.begin() + 1 + c.m};
            bs.seeds.erase(bs.seeds.begin(), bs.seeds.begin() + 1 + c.m);
            energy += 24;
        }
        else if (c.type == Command::Fill)
        {
            Coordinate fc = bs.c + c.cd1;
            correct = correct && matrix.IsInside(fc);
            assert(correct);
            energy += matrix.Get(fc) ? 6 : 12;
            matrix.Fill(fc);
            toAdd.push_back(matrix.Index(fc.x, fc.y, fc.z));
            ic.AddCoordinate(fc);
        }
        else if (c.type == Command::Void)
        {
            Coordinate fc = bs.c + c.cd1;
            correct = correct && matrix.IsInside(fc);
            assert(correct);
            energy += matrix.Get(fc) ? -12 : 3;
            matrix.Erase(fc);
            toDelete.push_back(matrix.Index(fc.x, fc.y, fc.z));
            ic.AddCoordinate(fc);
        }
        else if (c.type == Command::FusionP)
        {
            Coordinate pc = bs.c + c.cd1;
            unsigned bid2 = bid;
            for (unsigned jbid : bots)
            {
                BotState& jbs = all_bots[jbid];
                if (jbs.c == pc)
                {
                    // TODO:
                    //   Check command on the second bot
                    bid2 = jbid;
                    break;
                }
            }
            correct = correct && (bid2 != bid);
            assert(correct);
            bs.seeds.push_back(bid2);
            bs.seeds.insert(bs.seeds.end(), all_bots[bid2].seeds.begin(), all_bots[bid2].seeds.end());
            sort(bs.seeds.begin(), bs.seeds.end());
        }
        else if (c.type == Command::FusionS)
        {
            Coordinate pc = bs.c + c.cd1;
            bool found = false;
            for (unsigned jbid : bots)
            {
                BotState& jbs = all_bots[jbid];
                if (jbs.c == pc)
                {
                    // TODO:
                    //   Check command on the second bot
                    found = true;
                    break;
                }
            }
            correct = correct && found;
            assert(correct);
            auto it = find(active_bots.begin(), active_bots.end(), bid);
            assert(it != active_bots.end());
            active_bots.erase(it);
            energy -= 24;
        }
        else if (c.type == Command::GFill)
        {
            correct = correct && c.cd1.IsNearCoordinateDifferences() && c.cd2.IsFarCoordinateDifferences();
            Coordinate a = bs.c + c.cd1;
            Coordinate b = a + c.cd2;
            correct = correct && matrix.IsInside(a) && matrix.IsInside(b);
            fills[Region(a, b)].push_back(a);
        }
        else if (c.type == Command::GVoid)
        {
            correct = correct && c.cd1.IsNearCoordinateDifferences() && c.cd2.IsFarCoordinateDifferences();
            Coordinate a = bs.c + c.cd1;
            Coordinate b = a + c.cd2;
            correct = correct && matrix.IsInside(a) && matrix.IsInside(b);
            erases[Region(a, b)].push_back(a);
        }
    }
    for (auto const& fill : fills)
    {
        set<Coordinate> corners = fill.first.Corners();
        set<Coordinate> botCorners(fill.second.begin(), fill.second.end());
        correct = correct && (corners.size() == fill.second.size());
        correct = correct && (corners == botCorners);
        for (int x = fill.first.a.x; x <= fill.first.b.x; ++x)
        {
            for (int y = fill.first.a.y; y <= fill.first.b.y; ++y)
            {
                for (int z = fill.first.a.z; z <= fill.first.b.z; ++z)
                {
                    Coordinate fc{x, y, z};
                    energy += matrix.Get(fc) ? 6 : 12;
                    matrix.Fill(fc);
                    toAdd.push_back(matrix.Index(x, y, z));
                    ic.AddCoordinate(fc);
                }
            }
        }
    }

    for (auto const& erase : erases)
    {
        set<Coordinate> corners = erase.first.Corners();
        set<Coordinate> botCorners(erase.second.begin(), erase.second.end());
        correct = correct && (corners.size() == erase.second.size());
        correct = correct && (corners == botCorners);
        for (int x = erase.first.a.x; x <= erase.first.b.x; ++x)
        {
            for (int y = erase.first.a.y; y <= erase.first.b.y; ++y)
            {
                for (int z = erase.first.a.z; z <= erase.first.b.z; ++z)
                {
                    Coordinate fc{x, y, z};
                    energy += matrix.Get(fc) ? -12 : 3;
                    matrix.Erase(fc);
                    toDelete.push_back(matrix.Index(x, y, z));
                    ic.AddCoordinate(fc);
                }
            }
        }
    }

    if (hc & 1)
    {
        harmonics = !harmonics;
    }
    bool icValid = ic.IsValid();
    correct = correct && icValid;
    if (!cmd.int_args["levitation"]) {
        bool grounded = harmonics || IsGrounded();
        correct = correct && grounded;
        if (!correct) {
            trace_pos = trace.size();
            cerr << "[WARN] State is incorrect. Grounded = " << grounded << ", Trace_Pos = " << trace_pos
                 << ", HC = " << hc << ", icValid = " << icValid << endl;
            throw StopException();
        }
    }
    assert(correct);
}

void State::Run()
{
    for (; trace_pos < trace.size(); )
        Step();
}

bool State::MoveBot(BotState& bs, InterfereCheck& ic, const CoordinateDifference& cd)
{
    assert(cd.IsLinearCoordinateDifferences());
    CoordinateDifference step { sign(cd.dx), sign(cd.dy), sign(cd.dz) };
    unsigned l = cd.ManhattanLength();
    for (unsigned i = 1; i <= l; ++i)
    {
        bs.c += step;
        correct = correct && matrix.IsInside(bs.c) && !matrix.Get(bs.c);
        ic.AddCoordinate(bs.c);
        energy += 2;
    }
    return correct;
}
