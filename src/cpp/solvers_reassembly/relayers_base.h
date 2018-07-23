#pragma once

#include "../evaluation.h"
#include "../solvers_util.h"
#include "../state.h"

class ReassemblySolverLayersBase : public SolverBase {
   protected:
    Matrix source;
    Matrix target;

    ReassemblySolverLayersBase(const Matrix& source, const Matrix& target, bool levitation);

    bool NeedChange(const Coordinate& c) const;
    void Solve(Trace& output);

   public:
    static Evaluation::Result Solve(const Matrix& source, const Matrix& target, Trace& output, bool levitation);
};
