#define GOOGLE_TEST_MODULE test_main_google

#include "test_main_google.hpp"

#include <gtest/gtest.h>

const char* testArgv[] = {"path", "3"};

TEST(Test_static_input, Subtest_1)
{
    std::unique_ptr<LoggerFixedCntCMDs> genLogger = std::make_unique<LoggerFixedCntCMDs>(std::stoi(static_cast<std::string>(testArgv[1])));
    std::unique_ptr<LoggerRemainingCMDs> otherLogger = std::make_unique<LoggerRemainingCMDs>();

    std::string currCommand;
    std::queue<std::string> listCommands;
    listCommands.push("cmd1");
    listCommands.push("cmd2");
    listCommands.push("cmd3");
    listCommands.push("cmd4");
    listCommands.push("cmd5");
    size_t cntUnfinishedBraces = 0;
    size_t t_currCountCommands = 0;

    /*!
     *  Проверка работы логики отвечающей вывод при статическом вводе (в примере N == 3):

        ./bulk 3
        Ввод    Вывод                   Комментарий
        cmd1
        cmd2
        cmd3
                bulk: cmd1, cmd2, cmd3  Блок завершён – выводим блок.
        cmd4
        cmd5
        EOF
                bulk: cmd4, cmd5        Конец ввода – принудительно завершаем блок.
    */

    while (!listCommands.empty())
    {
        currCommand = listCommands.front();
        listCommands.pop();

        if (currCommand.empty())
        {   break;   }


        if (currCommand == "{")
        {
            ++cntUnfinishedBraces;

            if (cntUnfinishedBraces == 1)
            {
                EXPECT_EQ(genLogger->logMessage(LoggerFixedCntCMDs::Mode::REMAINING), "");
            }

            continue;
        }
        if (currCommand == "}")
        {
            --cntUnfinishedBraces;

            if (cntUnfinishedBraces == 0)
            {
                EXPECT_EQ(otherLogger->logMessage(), "");
            }

            continue;
        }

        EXPECT_EQ(cntUnfinishedBraces, 0);

        if (cntUnfinishedBraces >= 1)
        {
            otherLogger->pushCommand(currCommand);
            EXPECT_EQ(otherLogger->countCommands(), 0);
        }
        else
        {
            ++t_currCountCommands;
            genLogger->pushCommand(currCommand);
            EXPECT_EQ(genLogger->countCommands(), t_currCountCommands);
        }

        if (genLogger->countCommands() == 3)
        {
            EXPECT_EQ(genLogger->logMessage(), "bulk: cmd1, cmd2, cmd3\n");
            EXPECT_EQ(genLogger->countCommands(), 0);
            t_currCountCommands = 0;
        }
        else
        {
            EXPECT_EQ(genLogger->logMessage(), "");
        }
    }
    EXPECT_EQ(genLogger->countCommands(), 2);
    EXPECT_EQ(genLogger->logMessage(LoggerFixedCntCMDs::Mode::REMAINING), "bulk: cmd4, cmd5\n");
    EXPECT_EQ(genLogger->countCommands(), 0);
}


TEST(Test_dynamic_input, Subtest_2)
{
    std::unique_ptr<LoggerFixedCntCMDs> genLogger = std::make_unique<LoggerFixedCntCMDs>(std::stoi(static_cast<std::string>(testArgv[1])));
    std::unique_ptr<LoggerRemainingCMDs> otherLogger = std::make_unique<LoggerRemainingCMDs>();

    std::string currCommand;
    std::queue<std::string> listCommands;
    listCommands.push("cmd1");
    listCommands.push("cmd2");
    listCommands.push("{");
    listCommands.push("cmd3");
    listCommands.push("cmd4");
    listCommands.push("}");
    listCommands.push("{");
    listCommands.push("cmd5");
    listCommands.push("cmd6");
    listCommands.push("{");
    listCommands.push("cmd7");
    listCommands.push("cmd8");
    listCommands.push("}");
    listCommands.push("cmd9");
    listCommands.push("}");
    listCommands.push("{");
    listCommands.push("cmd10");
    listCommands.push("cmd11");

    size_t cntUnfinishedBraces = 0;
    size_t t_currCountGenCommands = 0;
    size_t t_currCountAddCommands = 0;

    /*!
     *  Проверка работы логики отвечающей вывод при динамическом вводе (в примере N == 3):

        ./bulk 3

        Ввод    Вывод                   Комментарий
        cmd1
        cmd2
        {
                bulk: cmd1, cmd2        Начало динамического блока – выводим предыдущий статический досрочно
        cmd3
        cmd4
        }
                bulk: cmd3, cmd4
        {
        cmd5
        cmd6
        {                               Игнорируем вложенные команды.
        cmd7
        cmd8
        }                               Игнорируем вложенные команды.
        cmd9
        }
                bulk: cmd5, cmd6,       Конец динамического блока – выводим.
                cmd7, cmd8, cmd9
        {
        cmd10                           Конец ввода – динамический блок игнорируется, не выводим
        cmd11
        EOF
    */

    while (!listCommands.empty())
    {
        currCommand = listCommands.front();
        listCommands.pop();

        if (currCommand.empty())
        {   break;   }


        if (currCommand == "{")
        {
            ++cntUnfinishedBraces;

            if (cntUnfinishedBraces == 1)
            {
                if (t_currCountGenCommands == 2)
                {
                    EXPECT_EQ(genLogger->logMessage(LoggerFixedCntCMDs::Mode::REMAINING), "bulk: cmd1, cmd2\n");
                }
                else
                {
                    EXPECT_EQ(genLogger->logMessage(LoggerFixedCntCMDs::Mode::REMAINING), "");
                }
                t_currCountGenCommands = 0;
                EXPECT_EQ(genLogger->countCommands(), t_currCountGenCommands);
            }

            continue;
        }
        if (currCommand == "}")
        {
            --cntUnfinishedBraces;

            if (cntUnfinishedBraces == 0)
            {
                if (t_currCountAddCommands == 2)
                {
                    EXPECT_EQ(otherLogger->logMessage(), "bulk: cmd3, cmd4\n");
                }
                else
                {
                    EXPECT_EQ(otherLogger->logMessage(), "bulk: cmd5, cmd6, cmd7, cmd8, cmd9\n");
                }
                t_currCountAddCommands = 0;
                EXPECT_EQ(genLogger->countCommands(), t_currCountAddCommands);
            }

            continue;
        }

        if (cntUnfinishedBraces >= 1)
        {
            ++t_currCountAddCommands;
            otherLogger->pushCommand(currCommand);
            EXPECT_EQ(otherLogger->countCommands(), t_currCountAddCommands);
        }
        else
        {
            ++t_currCountGenCommands;
            genLogger->pushCommand(currCommand);
            EXPECT_EQ(genLogger->countCommands(), t_currCountGenCommands);
        }

       EXPECT_EQ(genLogger->logMessage(), "");
    }
    EXPECT_EQ(genLogger->countCommands(), 0);
    EXPECT_EQ(genLogger->logMessage(LoggerFixedCntCMDs::Mode::REMAINING), "");
    EXPECT_EQ(genLogger->countCommands(), 0);
}
