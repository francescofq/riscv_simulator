#pragma once
#include<bits/stdc++.h>
#include "cpu.h"

namespace rv {

    class Cli {
    public:
        void run(const std::string& initialProgram = "");

    private:
        using Args = std::vector<std::string>;

        void dispatch(const Args& args);

        void cmdHelp() const;
        void cmdLoad(const Args& args);
        void cmdStep(const Args& args);
        void cmdRun(const Args& args);
        void cmdPc(const Args& args) const;
        void cmdRegs(const Args& args) const;
        void cmdMem(const Args& args) const;
        void cmdReset(const Args& args);

        Cpu cpu_;
        bool quit_ = false;
    };

}