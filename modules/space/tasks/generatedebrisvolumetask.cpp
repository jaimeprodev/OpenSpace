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
#include <modules/space/tasks/generatedebrisvolumetask.h>

#include <modules/volume/rawvolume.h>
#include <modules/volume/rawvolumemetadata.h>
//#include <modules/volume/rawvolumewriter.h>

#include <openspace/documentation/verifier.h>


#include <ghoul/filesystem/filesystem.h>
#include <ghoul/filesystem/file.h>
//#include <ghoul/misc/dictionaryluaformatter.h>
//#include <ghoul/misc/defer.h>

#include <fstream>


namespace {
    constexpr const char* KeyRawVolumeOutput = "RawVolumeOutput";
    constexpr const char* KeyDictionaryOutput = "DictionaryOutput";
    constexpr const char* KeyDimensions = "Dimensions";
    constexpr const char* KeyStartTime = "StartTime";
    constexpr const char* KeyEndTime = "EndTime";
    constexpr const char* KeyInputPath = "InputPath";
    // constexpr const char* KeyLowerDomainBound = "LowerDomainBound";
    // constexpr const char* KeyUpperDomainBound = "UpperDomainBound";    

}

namespace openspace{
namespace volume {
// The list of leap years only goes until 2056 as we need to touch this file then
// again anyway ;)
const std::vector<int> LeapYears = {
    1956, 1960, 1964, 1968, 1972, 1976, 1980, 1984, 1988, 1992, 1996,
    2000, 2004, 2008, 2012, 2016, 2020, 2024, 2028, 2032, 2036, 2040,
    2044, 2048, 2052, 2056
};
// Count the number of full days since the beginning of 2000 to the beginning of
// the parameter 'year'
int countDays(int year) {
    // Find the position of the current year in the vector, the difference
    // between its position and the position of 2000 (for J2000) gives the
    // number of leap years
    constexpr const int Epoch = 2000;
    constexpr const int DaysRegularYear = 365;
    constexpr const int DaysLeapYear = 366;

    if (year == Epoch) {
        return 0;
    }

    // Get the position of the most recent leap year
    const auto lb = std::lower_bound(LeapYears.begin(), LeapYears.end(), year);

    // Get the position of the epoch
    const auto y2000 = std::find(LeapYears.begin(), LeapYears.end(), Epoch);

    // The distance between the two iterators gives us the number of leap years
    const int nLeapYears = static_cast<int>(std::abs(std::distance(y2000, lb)));

    const int nYears = std::abs(year - Epoch);
    const int nRegularYears = nYears - nLeapYears;

    // Get the total number of days as the sum of leap years + non leap years
    const int result = nRegularYears * DaysRegularYear + nLeapYears * DaysLeapYear;
    return result;
}

// Returns the number of leap seconds that lie between the {year, dayOfYear}
// time point and { 2000, 1 }
int countLeapSeconds(int year, int dayOfYear) {
    // Find the position of the current year in the vector; its position in
    // the vector gives the number of leap seconds
    struct LeapSecond {
        int year;
        int dayOfYear;
        bool operator<(const LeapSecond& rhs) const {
            return std::tie(year, dayOfYear) < std::tie(rhs.year, rhs.dayOfYear);
        }
    };

    const LeapSecond Epoch = { 2000, 1 };

    // List taken from: https://www.ietf.org/timezones/data/leap-seconds.list
    static const std::vector<LeapSecond> LeapSeconds = {
        { 1972,   1 },
        { 1972, 183 },
        { 1973,   1 },
        { 1974,   1 },
        { 1975,   1 },
        { 1976,   1 },
        { 1977,   1 },
        { 1978,   1 },
        { 1979,   1 },
        { 1980,   1 },
        { 1981, 182 },
        { 1982, 182 },
        { 1983, 182 },
        { 1985, 182 },
        { 1988,   1 },
        { 1990,   1 },
        { 1991,   1 },
        { 1992, 183 },
        { 1993, 182 },
        { 1994, 182 },
        { 1996,   1 },
        { 1997, 182 },
        { 1999,   1 },
        { 2006,   1 },
        { 2009,   1 },
        { 2012, 183 },
        { 2015, 182 },
        { 2017,   1 }
    };

    // Get the position of the last leap second before the desired date
    LeapSecond date { year, dayOfYear };
    const auto it = std::lower_bound(LeapSeconds.begin(), LeapSeconds.end(), date);

    // Get the position of the Epoch
    const auto y2000 = std::lower_bound(
        LeapSeconds.begin(),
        LeapSeconds.end(),
        Epoch
    );

    // The distance between the two iterators gives us the number of leap years
    const int nLeapSeconds = static_cast<int>(std::abs(std::distance(y2000, it)));
    return nLeapSeconds;
}

double calculateSemiMajorAxis(double meanMotion) {
    constexpr const double GravitationalConstant = 6.6740831e-11;
    constexpr const double MassEarth = 5.9721986e24;
    constexpr const double muEarth = GravitationalConstant * MassEarth;

    // Use Kepler's 3rd law to calculate semimajor axis
    // a^3 / P^2 = mu / (2pi)^2
    // <=> a = ((mu * P^2) / (2pi^2))^(1/3)
    // with a = semimajor axis
    // P = period in seconds
    // mu = G*M_earth
    double period = std::chrono::seconds(std::chrono::hours(24)).count() / meanMotion;

    const double pisq = glm::pi<double>() * glm::pi<double>();
    double semiMajorAxis = pow((muEarth * period*period) / (4 * pisq), 1.0 / 3.0);

    // We need the semi major axis in km instead of m
    return semiMajorAxis / 1000.0;
}

double epochFromSubstring(const std::string& epochString) {
    // The epochString is in the form:
    // YYDDD.DDDDDDDD
    // With YY being the last two years of the launch epoch, the first DDD the day
    // of the year and the remaning a fractional part of the day

    // The main overview of this function:
    // 1. Reconstruct the full year from the YY part
    // 2. Calculate the number of seconds since the beginning of the year
    // 2.a Get the number of full days since the beginning of the year
    // 2.b If the year is a leap year, modify the number of days
    // 3. Convert the number of days to a number of seconds
    // 4. Get the number of leap seconds since January 1st, 2000 and remove them
    // 5. Adjust for the fact the epoch starts on 1st Januaray at 12:00:00, not
    // midnight

    // According to https://celestrak.com/columns/v04n03/
    // Apparently, US Space Command sees no need to change the two-line element
    // set format yet since no artificial earth satellites existed prior to 1957.
    // By their reasoning, two-digit years from 57-99 correspond to 1957-1999 and
    // those from 00-56 correspond to 2000-2056. We'll see each other again in 2057!

    // 1. Get the full year
    std::string yearPrefix = [y = epochString.substr(0, 2)](){
        int year = std::atoi(y.c_str());
        return year >= 57 ? "19" : "20";
    }();
    const int year = std::atoi((yearPrefix + epochString.substr(0, 2)).c_str());
    const int daysSince2000 = countDays(year);

    // 2.
    // 2.a
    double daysInYear = std::atof(epochString.substr(2).c_str());

    // 2.b
    const bool isInLeapYear = std::find(
        LeapYears.begin(),
        LeapYears.end(),
        year
    ) != LeapYears.end();
    if (isInLeapYear && daysInYear >= 60) {
        // We are in a leap year, so we have an effective day more if we are
        // beyond the end of february (= 31+29 days)
        --daysInYear;
    }

    // 3
    using namespace std::chrono;
    const int SecondsPerDay = static_cast<int>(seconds(hours(24)).count());
    //Need to subtract 1 from daysInYear since it is not a zero-based count
    const double nSecondsSince2000 = (daysSince2000 + daysInYear - 1) * SecondsPerDay;

    // 4
    // We need to remove additionbal leap seconds past 2000 and add them prior to
    // 2000 to sync up the time zones
    const double nLeapSecondsOffset = -countLeapSeconds(
        year,
        static_cast<int>(std::floor(daysInYear))
    );

    // 5
    const double nSecondsEpochOffset = static_cast<double>(
        seconds(hours(12)).count()
    );

    // Combine all of the values
    const double epoch = nSecondsSince2000 + nLeapSecondsOffset - nSecondsEpochOffset;
    return epoch;
}

std::vector<KeplerParameters> readTLEFile(const std::string& filename){
    ghoul_assert(FileSys.fileExists(filename), "The filename must exist");
    
    std::vector<KeplerParameters> data;

    std::ifstream file;
    file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    file.open(filename);

    int numberOfLines = std::count(std::istreambuf_iterator<char>(file), 
                                   std::istreambuf_iterator<char>(), '\n' );
    file.seekg(std::ios_base::beg); // reset iterator to beginning of file

    // 3 because a TLE has 3 lines per element/ object.
    int numberOfObjects = numberOfLines/3;

    std::string line = "-";
    for (int i = 0; i < numberOfObjects; i++) {

        std::getline(file, line); // get rid of title
        
        KeplerParameters keplerElements;

        std::getline(file, line);
        if (line[0] == '1') {
            // First line
            // Field Columns   Content
            //     1   01-01   Line number
            //     2   03-07   Satellite number
            //     3   08-08   Classification (U = Unclassified)
            //     4   10-11   International Designator (Last two digits of launch year)
            //     5   12-14   International Designator (Launch number of the year)
            //     6   15-17   International Designator(piece of the launch)    A
            //     7   19-20   Epoch Year(last two digits of year)
            //     8   21-32   Epoch(day of the year and fractional portion of the day)
            //     9   34-43   First Time Derivative of the Mean Motion divided by two
            //    10   45-52   Second Time Derivative of Mean Motion divided by six
            //    11   54-61   BSTAR drag term(decimal point assumed)[10] - 11606 - 4
            //    12   63-63   The "Ephemeris type"
            //    13   65-68   Element set  number.Incremented when a new TLE is generated
            //    14   69-69   Checksum (modulo 10)
            keplerElements.epoch = epochFromSubstring(line.substr(18, 14));
        }
        else {
            throw ghoul::RuntimeError(fmt::format(
                "File {} @ line {} does not have '1' header", filename // linNum + 1
            ));
        }

        std::getline(file, line);
        if (line[0] == '2') {
            // Second line
            // Field    Columns   Content
            //     1      01-01   Line number
            //     2      03-07   Satellite number
            //     3      09-16   Inclination (degrees)
            //     4      18-25   Right ascension of the ascending node (degrees)
            //     5      27-33   Eccentricity (decimal point assumed)
            //     6      35-42   Argument of perigee (degrees)
            //     7      44-51   Mean Anomaly (degrees)
            //     8      53-63   Mean Motion (revolutions per day)
            //     9      64-68   Revolution number at epoch (revolutions)
            //    10      69-69   Checksum (modulo 10)

            std::stringstream stream;
            stream.exceptions(std::ios::failbit);

            // Get inclination
            stream.str(line.substr(8, 8));
            stream >> keplerElements.inclination;
            stream.clear();

            // Get Right ascension of the ascending node
            stream.str(line.substr(17, 8));
            stream >> keplerElements.ascendingNode;
            stream.clear();

            // Get Eccentricity
            stream.str("0." + line.substr(26, 7));
            stream >> keplerElements.eccentricity;
            stream.clear();

            // Get argument of periapsis
            stream.str(line.substr(34, 8));
            stream >> keplerElements.argumentOfPeriapsis;
            stream.clear();

            // Get mean anomaly
            stream.str(line.substr(43, 8));
            stream >> keplerElements.meanAnomaly;
            stream.clear();

            // Get mean motion
            stream.str(line.substr(52, 11));
            stream >> keplerElements.meanMotion;
        }
        else {
            throw ghoul::RuntimeError(fmt::format(
                "File {} @ line {} does not have '2' header", filename  // , lineNum + 2
            ));
        }

        // Calculate the semi major axis based on the mean motion using kepler's laws
        keplerElements.semiMajorAxis = calculateSemiMajorAxis(keplerElements.meanMotion);

        using namespace std::chrono;
        double period = seconds(hours(24)).count() / keplerElements.meanMotion;
        keplerElements.period = period;

        data.push_back(keplerElements);

    } // !for loop
    file.close();
    return data;
}

std::vector<glm::dvec3> getPositionBuffer(std::vector<KeplerParameters> tleData, std::string& timeStamp) {

    std::vector<glm::dvec3> positionBuffer;
    for(const auto& orbit : tleData) {
        KeplerTranslation keplerTranslator;
        keplerTranslator.setKeplerElements(
            orbit.eccentricity,
            orbit.semiMajorAxis,
            orbit.inclination,
            orbit.ascendingNode,
            orbit.argumentOfPeriapsis,
            orbit.meanAnomaly,
            orbit.period,
            orbit.epoch
        );    
        double timeInSeconds = Time::convertTime(timeStamp);
        glm::dvec3 position = keplerTranslator.debrisPos(static_cast<float>(timeInSeconds));
        positionBuffer.push_back(position);
        
    }
    
    return positionBuffer;
}

std::vector<glm::dvec3> generatePositions(int numberOfPositions) {
    std::vector<glm::dvec3> positions;
    
    float radius = 700000;   // meter
    float degreeStep = 360 / numberOfPositions;

    for(int i=0 ; i<= 360 ; i == degreeStep){
        glm::dvec3 singlePosition = glm::dvec3(radius* sin(i), radius*cos(i), 0.0);
        positions.push_back(singlePosition);
    }
    return positions;
}

float getDensityAt(glm::uvec3 cell) {
    float value;
    // return value at position cell from _densityPerVoxel
    return value;
}

float getMaxApogee(std::vector<KeplerParameters> inData){
    double maxApogee = 0.0;
    for (const auto& dataElement : inData){
        double ah = dataElement.semiMajorAxis * (1 + dataElement.eccentricity)- 6371;
        if (ah > maxApogee)
            maxApogee = ah;
    }   

    return static_cast<float>(maxApogee);
}

int getIndexFromPosition(glm::dvec3 position, glm::uvec3 dim, float maxApogee){
    // epsilon is to make sure that for example if newPosition.x/maxApogee = 1,
    // then the index for that dimension will not exceed the range of the grid.
    float epsilon = static_cast<float>(0.000000001);
    glm::vec3 newPosition = glm::vec3(position.x + maxApogee
                                     ,position.y + maxApogee
                                     ,position.z + maxApogee);

    glm::vec3 coordinateIndex = glm::vec3(newPosition.x * dim.x, 
                                          newPosition.y * dim.y, 
                                          newPosition.z * dim.z) / (2*(maxApogee + epsilon));

    
    return coordinateIndex.z * (dim.x * dim.y) + coordinateIndex.y * dim.x + coordinateIndex.x;
}

int* mapDensityToVoxels(int* densityArray, std::vector<glm::dvec3> positions,
                         glm::uvec3 dim, float maxApogee) {

    for(const glm::dvec3& position : positions) {
        int index = getIndexFromPosition(position, dim, maxApogee);
        ++densityArray[index];
    }
    return densityArray;
}

GenerateDebrisVolumeTask::GenerateDebrisVolumeTask(const ghoul::Dictionary& dictionary)
{
    openspace::documentation::testSpecificationAndThrow(
        documentation(),
        dictionary,
        "GenerateDebrisVolumeTask"
    );

    _rawVolumeOutputPath = absPath(dictionary.value<std::string>(KeyRawVolumeOutput));
    _dictionaryOutputPath = absPath(dictionary.value<std::string>(KeyDictionaryOutput));
    _dimensions = glm::uvec3(dictionary.value<glm::vec3>(KeyDimensions));
    _startTime = dictionary.value<std::string>(KeyStartTime);
    _endTime = dictionary.value<std::string>(KeyEndTime);
    // since _inputPath is past from task,
    // there will have to be either one task per dataset,
    // or you need to combine the datasets into one file.
    _inputPath = dictionary.value<std::string>(KeyInputPath);
    //_lowerDomainBound = dictionary.value<glm::vec3>(KeyLowerDomainBound);
    //_upperDomainBound = dictionary.value<glm::vec3>(KeyUpperDomainBound);


    _TLEDataVector = {};
    _lowerDomainBound = dictionary.value<glm::vec3>(KeyLowerDomainBound);
    _upperDomainBound = dictionary.value<glm::vec3>(KeyUpperDomainBound);
}

std::string GenerateDebrisVolumeTask::description() {
    return "todo:: description";
}

void GenerateDebrisVolumeTask::perform(const Task::ProgressCallback& progressCallback) {

    //1. read TLE-data and position of debris elements.
    _TLEDataVector = readTLEFile(_inputPath);
    std::vector<glm::dvec3> startPositionBuffer = getPositionBuffer(_TLEDataVector, _startTime);
    //  if we deside to integrate the density over time
    //  std::vector<glm::dvec3> endPositionBuffer = getPositionBuffer(_TLEDataVector, _endTime);
    
    int numberOfPoints = 40;
    std::vector<glm::dvec3> generatedPositions = generatePositions(numberOfPoints);
    
    const int size = _dimensions.x *_dimensions.y *_dimensions.z;
    int *densityArrayp = new int[size];
    float maxApogee = getMaxApogee(_TLEDataVector);
    densityArrayp = mapDensityToVoxels(densityArrayp, generatedPositions, _dimensions, maxApogee);
        
    // create object rawVolume
    volume::RawVolume<float> rawVolume(_dimensions);
    glm::vec3 domainSize = _upperDomainBound - _lowerDomainBound;
    
    float minVal = std::numeric_limits<float>::max();
    float maxVal = std::numeric_limits<float>::min();
    
    // TODO: Create a forEachSatallite and set(cell, value) to combine mapDensityToVoxel
    //      and forEachVoxel for less time complexity.
    rawVolume.forEachVoxel([&](glm::uvec3 cell, float) {
    //     glm::vec3 coord = _lowerDomainBound +
    //        glm::vec3(cell) / glm::vec3(_dimensions) * domainSize;
        float value = getDensityAt(cell);   // (coord)
        rawVolume.set(cell, value);

        minVal = std::min(minVal, value);
        maxVal = std::max(maxVal, value);
    });


  

    






    // indexed grid with cooresponding xyz: gridIndex
    // int array with number of slots = number of voxels: densityArray = {0,0,0,0,0,0}
    for(const glm::dvec3& debris : startPositionBuffer) {
        // call function to increment densityArray where debris position ~= voxel position
    }
    // make raw volume of densityArray.

    //2. create a grid using dimensions and other .task-parameters.

    //3. calculate what voxel each debris is within for each time step.
    //   and increment density with one for each debris in that voxel.

    glm::vec3 domainSize = _upperDomainBound - _lowerDomainBound;

    rawVolume.forEachVoxel([&](glm::uvec3 cell, float) {
        glm::vec3 coord = _lowerDomainBound +
            glm::vec3(cell) / glm::vec3(_dimensions) * domainSize;

        // TODO: coord is relative to the dimensions of the volume
        // and the points from getPositionBuffer is relative
        // to the earth (i think) so we need to convert them to be in the same
        // coordinate system to be able to compare them

        

        
    });
    //4.
    

















}

documentation::Documentation GenerateDebrisVolumeTask::documentation() {
    using namespace documentation;
    return {
        "GenerateDebrisVolumeTask",
        "generate_debris_volume_task",
        {
            {
                "Type",
                new StringEqualVerifier("GenerateDebrisVolumeTask"),
                Optional::No,
                "The type of this task",
            },
            // {
            //     KeyValueFunction,
            //     new StringAnnotationVerifier("A lua expression that returns a function "
            //     "taking three numbers as arguments (x, y, z) and returning a number."),
            //     Optional::No,
            //     "The lua function used to compute the cell values",
            // },
            {
                KeyRawVolumeOutput,
                new StringAnnotationVerifier("A valid filepath"),
                Optional::No,
                "The raw volume file to export data to",
            },
            {
                KeyDictionaryOutput,
                new StringAnnotationVerifier("A valid filepath"),
                Optional::No,
                "The lua dictionary file to export metadata to",
            },
            {
                KeyDimensions,
                new DoubleVector3Verifier,
                Optional::No,
                "A vector representing the number of cells in each dimension",
            },
            // {
            //     KeyLowerDomainBound,
            //     new DoubleVector3Verifier,
            //     Optional::No,
            //     "A vector representing the lower bound of the domain"
            // },
            // {
            //     KeyUpperDomainBound,
            //     new DoubleVector3Verifier,
            //     Optional::No,
            //     "A vector representing the upper bound of the domain"
            // }
        }
    };
}

} // namespace volume
} // namespace openspace
