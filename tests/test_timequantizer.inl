/*****************************************************************************************
 *                                                                                       *
 * OpenSpace                                                                             *
 *                                                                                       *
 * Copyright (c) 2014-2019                                                               *
 *                                                                                       *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this  *
 * software and associated documentation files (the "Software"), to deal in the Software *
 * without restriction, including without limitation the rights to use, copy, modify,    *
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to    *
 * permit persons to whom the Software is furnished to do so, subject to the following   *
 * conditions:                                                                           *
 *                                                                                       *
 * The above copyright notice and this permission notice shall be included in all copies *
 * or substantial portions of the Software.                                              *
 *                                                                                       *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,   *
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A         *
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT    *
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF  *
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE  *
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                                         *
 ****************************************************************************************/

#include "gtest/gtest.h"
#include <openspace/util/time.h>
#include "modules/globebrowsing/src/timequantizer.h"
#include <openspace/util/spicemanager.h>
#include <ghoul/filesystem/filesystem.h>
#include "SpiceUsr.h"
#include "SpiceZpr.h"

#define _USE_MATH_DEFINES
#include <math.h>
#include <glm/glm.hpp>

class TimeQuantizerTest : public testing::Test {
protected:
    void SetUp() override {
        openspace::SpiceManager::initialize();
    }

    void TearDown() override {
        openspace::SpiceManager::deinitialize();

    }
};

//global constants 
#define  FILLEN   128
#define  TYPLEN   32
#define  SRCLEN   128

namespace spicemanager_constants {
    const int nrMetaKernels = 9;
    SpiceInt which, handle, count = 0;
    char file[FILLEN], filtyp[TYPLEN], source[SRCLEN];
    double abs_error = 0.00001;
} // namespace spicemanager_constants

#define LSK "C:\\Users\\OpenSpace\\Desktop\\profiles\\OpenSpace\\tests\\SpiceTest\\spicekernels\\naif0008.tls"

int loadLSKKernel() {
    int kernelID = openspace::SpiceManager::ref().loadKernel(
        std::string(LSK)
    );
    EXPECT_EQ(1, kernelID) << "loadKernel did not return proper id";
    return kernelID;
}

static void singleTimeTest(openspace::Time& t,
                           openspace::globebrowsing::TimeQuantizer& tq, bool clamp,
                           const std::string& input, const std::string& expected)
{
    t.setTime(input);
    tq.quantize(t, clamp);
    EXPECT_EQ(t.ISO8601(), expected);
}

static void singleResolutionTest(openspace::globebrowsing::TimeQuantizer& tq,
                                 std::string resolution, std::string expectedType,
                                 bool expectFailure)
{
    std::string res;
    std::string search = "Invalid resolution ";
    try {
        tq.setResolution(resolution);
    }
    catch (const ghoul::RuntimeError& e) {
        res = e.message;
    }

    if (expectFailure) {
        EXPECT_TRUE(res.find(search) != std::string::npos);
        EXPECT_TRUE(res.find(expectedType) != std::string::npos);
    }
    else {
        EXPECT_TRUE(res.find(search) == std::string::npos);
    }
}

static void LoadSpiceKernel () {
    loadLSKKernel();
    // naif0008.tls is a text file, check if loaded.
    SpiceBoolean found;
    kdata_c(
        0,
        "text",
        FILLEN,
        TYPLEN,
        SRCLEN,
        spicemanager_constants::file,
        spicemanager_constants::filtyp,
        spicemanager_constants::source,
        &spicemanager_constants::handle,
        &found
    );

    ASSERT_TRUE(found == SPICETRUE) << "Kernel not loaded";
}

TEST_F(TimeQuantizerTest, TestYears) {
    LoadSpiceKernel();
    using namespace openspace::globebrowsing;
    TimeQuantizer t1;
    openspace::Time testT;

    t1.setStartEndRange("2019-12-09T00:00:00", "2030-03-01T00:00:00");
    t1.setResolution("1y");

    singleTimeTest(testT, t1, true, "2020-12-08T23:59:59", "2019-12-09T00:00:00.000");
    singleTimeTest(testT, t1, true, "2020-12-09T00:00:00", "2020-12-09T00:00:00.000");
    singleTimeTest(testT, t1, true, "2021-12-08T23:59:58", "2020-12-09T00:00:00.000");
    singleTimeTest(testT, t1, true, "2022-12-09T00:00:02", "2022-12-09T00:00:00.000");
    singleTimeTest(testT, t1, true, "2022-11-08T13:00:15", "2021-12-09T00:00:00.000");
    singleTimeTest(testT, t1, true, "2020-12-09T00:00:00", "2020-12-09T00:00:00.000");
    singleTimeTest(testT, t1, true, "2024-12-08T23:59:59", "2023-12-09T00:00:00.000");
    singleTimeTest(testT, t1, true, "2024-12-09T00:00:01", "2024-12-09T00:00:00.000");
    singleTimeTest(testT, t1, true, "2020-12-31T00:00:01", "2020-12-09T00:00:00.000");
    singleTimeTest(testT, t1, true, "2021-01-01T00:00:00", "2020-12-09T00:00:00.000");
    singleTimeTest(testT, t1, true, "2020-12-31T23:59:59", "2020-12-09T00:00:00.000");

    t1.setResolution("3y");

    singleTimeTest(testT, t1, true, "2020-12-08T23:59:59", "2019-12-09T00:00:00.000");
    singleTimeTest(testT, t1, true, "2022-12-09T00:00:00", "2022-12-09T00:00:00.000");
    singleTimeTest(testT, t1, true, "2028-12-08T23:59:59", "2025-12-09T00:00:00.000");
    singleTimeTest(testT, t1, true, "2028-12-09T00:00:01", "2028-12-09T00:00:00.000");
}

TEST_F(TimeQuantizerTest, TestDays) {
    LoadSpiceKernel();
    using namespace openspace::globebrowsing;
    TimeQuantizer t1;
    openspace::Time testT;

    t1.setStartEndRange("2019-12-09T00:00:00", "2020-03-01T00:00:00");
    t1.setResolution("1d");

    singleTimeTest(testT, t1, true, "2020-01-07T05:15:45", "2020-01-07T00:00:00.000");
    singleTimeTest(testT, t1, true, "2020-01-07T00:00:00", "2020-01-07T00:00:00.000");
    singleTimeTest(testT, t1, true, "2020-01-07T00:00:01", "2020-01-07T00:00:00.000");
    singleTimeTest(testT, t1, true, "2020-01-06T23:59:59", "2020-01-06T00:00:00.000");
    singleTimeTest(testT, t1, true, "2020-01-31T23:59:59", "2020-01-31T00:00:00.000");
    singleTimeTest(testT, t1, true, "2020-02-01T00:00:00", "2020-02-01T00:00:00.000");
    singleTimeTest(testT, t1, false, "2020-02-01T00:00:00", "2020-02-01T00:00:00.000");
    singleTimeTest(testT, t1, true, "2020-02-29T00:00:02", "2020-02-29T00:00:00.000");
    singleTimeTest(testT, t1, true, "2020-01-01T00:00:00", "2020-01-01T00:00:00.000");
    singleTimeTest(testT, t1, true, "2020-03-02T14:00:00", "2020-03-01T00:00:00.000");
    singleTimeTest(testT, t1, true, "2019-12-15T14:00:00", "2019-12-15T00:00:00.000");
    singleTimeTest(testT, t1, true, "2019-12-08T23:59:00", "2019-12-09T00:00:00.000");
    singleTimeTest(testT, t1, true, "2019-12-05T14:29:00", "2019-12-09T00:00:00.000");

    t1.setStartEndRange("2016-05-28T00:00:00", "2021-09-01T00:00:00");
    t1.setResolution("4d");

    singleTimeTest(testT, t1, true, "2016-06-01T00:00:00", "2016-06-01T00:00:00.000");
    singleTimeTest(testT, t1, true, "2016-06-01T00:00:01", "2016-06-01T00:00:00.000");
    singleTimeTest(testT, t1, true, "2016-07-03T10:00:00", "2016-07-03T00:00:00.000");
    singleTimeTest(testT, t1, true, "2016-07-07T00:00:00", "2016-07-07T00:00:00.000");
    singleTimeTest(testT, t1, true, "2021-11-07T00:00:00", "2021-09-01T00:00:00.000");
    singleTimeTest(testT, t1, false, "2021-11-07T00:00:00", "2021-11-07T00:00:00.000");

    t1.setStartEndRange("2019-02-21T00:00:00", "2021-09-01T00:00:00");
    t1.setResolution("11d");

    singleTimeTest(testT, t1, true, "2020-03-01T00:30:00", "2020-03-01T00:00:00.000");
    singleTimeTest(testT, t1, true, "2019-03-04T00:00:02", "2019-03-04T00:00:00.000");
}

TEST_F(TimeQuantizerTest, TestMonths) {
    LoadSpiceKernel();
    using namespace openspace::globebrowsing;
    TimeQuantizer t1;
    openspace::Time testT;

    t1.setStartEndRange("2017-01-28T00:00:00", "2020-09-01T00:00:00");
    t1.setResolution("1M");

    singleTimeTest(testT, t1, true, "2017-03-03T05:15:45", "2017-02-28T00:00:00.000");
    singleTimeTest(testT, t1, true, "2017-03-29T00:15:45", "2017-03-28T00:00:00.000");

    t1.setStartEndRange("2016-01-17T00:00:00", "2020-09-01T00:00:00");
    t1.setResolution("2M");

    singleTimeTest(testT, t1, true, "2016-01-27T05:15:45", "2016-01-17T00:00:00.000");
    singleTimeTest(testT, t1, true, "2016-03-16T08:15:45", "2016-01-17T00:00:00.000");
    singleTimeTest(testT, t1, true, "2016-03-17T18:00:02", "2016-03-17T00:00:00.000");
    singleTimeTest(testT, t1, true, "2016-05-18T00:00:02", "2016-05-17T00:00:00.000");
    singleTimeTest(testT, t1, true, "2016-11-17T10:15:45", "2016-11-17T00:00:00.000");
    singleTimeTest(testT, t1, true, "2017-01-18T05:15:45", "2017-01-17T00:00:00.000");

    t1.setResolution("3M");

    singleTimeTest(testT, t1, true, "2016-04-16T05:15:45", "2016-01-17T00:00:00.000");
    singleTimeTest(testT, t1, true, "2016-07-27T05:15:45", "2016-07-17T00:00:00.000");
    singleTimeTest(testT, t1, true, "2017-10-17T00:01:00", "2017-10-17T00:00:00.000");

    t1.setStartEndRange("2016-05-28T00:00:00", "2021-09-01T00:00:00");
    t1.setResolution("6M");

    singleTimeTest(testT, t1, true, "2016-11-28T00:00:05", "2016-11-28T00:00:00.000");
    singleTimeTest(testT, t1, true, "2017-05-30T04:15:45", "2017-05-28T00:00:00.000");
    singleTimeTest(testT, t1, true, "2017-10-17T05:01:00", "2017-05-28T00:00:00.000");
}

TEST_F(TimeQuantizerTest, TestTimes) {
    LoadSpiceKernel();
    using namespace openspace::globebrowsing;
    TimeQuantizer t1;
    openspace::Time testT;

    t1.setStartEndRange("2019-02-21T00:00:00", "2021-09-01T00:00:00");
    t1.setResolution("2h");

    singleTimeTest(testT, t1, true, "2019-02-28T16:10:00", "2019-02-28T16:00:00.000");
    singleTimeTest(testT, t1, true, "2019-02-28T22:00:00", "2019-02-28T22:00:00.000");

    t1.setResolution("3h");

    singleTimeTest(testT, t1, true, "2019-02-28T21:10:00", "2019-02-28T21:00:00.000");
    singleTimeTest(testT, t1, true, "2019-02-28T12:00:00", "2019-02-28T12:00:00.000");

    t1.setStartEndRange("2019-02-21T00:00:00", "2021-09-01T00:00:00");
    t1.setResolution("30m");

    singleTimeTest(testT, t1, true, "2019-02-27T16:40:00", "2019-02-27T16:30:00.000");
    singleTimeTest(testT, t1, true, "2019-02-28T22:00:00", "2019-02-28T22:00:00.000");
    singleTimeTest(testT, t1, true, "2019-02-28T22:30:01", "2019-02-28T22:30:00.000");
    singleTimeTest(testT, t1, true, "2019-02-28T21:29:59", "2019-02-28T21:00:00.000");
    singleTimeTest(testT, t1, true, "2019-02-28T22:59:59", "2019-02-28T22:30:00.000");

    t1.setResolution("15m");

    singleTimeTest(testT, t1, true, "2019-02-28T16:40:00", "2019-02-28T16:30:00.000");
    singleTimeTest(testT, t1, true, "2019-02-28T22:00:00", "2019-02-28T22:00:00.000");
    singleTimeTest(testT, t1, true, "2019-02-28T22:30:01", "2019-02-28T22:30:00.000");
    singleTimeTest(testT, t1, true, "2019-02-28T22:15:01", "2019-02-28T22:15:00.000");
    singleTimeTest(testT, t1, true, "2019-02-28T22:29:59", "2019-02-28T22:15:00.000");
    singleTimeTest(testT, t1, true, "2019-02-28T22:59:59", "2019-02-28T22:45:00.000");
}

TEST_F(TimeQuantizerTest, TestResolutionError) {
    LoadSpiceKernel();
    using namespace openspace::globebrowsing;
    TimeQuantizer t1;

    singleResolutionTest(t1, "29d", "(d)ay option.", true);
    singleResolutionTest(t1, "0d", "(d)ay option.", true);
    singleResolutionTest(t1, "5h", "(h)our option.", true);
    singleResolutionTest(t1, "11h", "(h)our option.", true);
    singleResolutionTest(t1, "12h", "(h)our option.", false);
    singleResolutionTest(t1, "78y", "(y)ear option.", false);
    singleResolutionTest(t1, "12m", "(m)inute option.", true);
    singleResolutionTest(t1, "1m", "(m)inute option.", true);
    singleResolutionTest(t1, "0m", "(m)inute option.", true);
    singleResolutionTest(t1, "15m", "(m)inute option.", false);
    singleResolutionTest(t1, "30m", "(m)inute option.", false);
    singleResolutionTest(t1, "31m", "(m)inute option.", true);
    singleResolutionTest(t1, "10s", "unit format", true);
}
