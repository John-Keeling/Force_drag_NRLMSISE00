/*! @file Force_drag_nrlmsise00.cpp
	@author John Keeling
	@date 11 October 2021
	@brief UCL ODL implementation of drag force model using nrlmsise00
 */

#include "../include/Resident_space_object.h"
#include "../include/Resident_variables.h"
#include <unistd.h>

void Force_drag_nrlmsise00::setup(const Resident_constants &rso_const,
                              std::shared_ptr<Resident_variables> in_state) {
     Force_drag::setup(rso_const, in_state);
}

void Force_drag_nrlmsise00::compute_acceleration() {
    // Report error, this displays the issue to the user and halts simulation
    if (state->geodetic.alt < 100.0) {
        std::stringstream error;
        error.precision(16);
        error << "Force_drag: Altitude too low, " << state->geodetic.alt
              << " km.";
        state->errors.push_back(error.str());
    }

    // USSA up to 1000 km, with exponential decay function above
    double rho = nrlmsise00_density(state->geodetic.alt);
    state->atmos_density = rho;
    a_ecef = minus500C_dAm * rho * state->ecef_v * state->ecef_rso_vel;
    state->total_a_ecef += a_ecef;
}

double Force_drag_nrlmsise00::nrlmsise00_density(double alt) {
    // Retrieves mass density from atmos. model for given time and location
    
    std::string model_latitude, model_longitude, model_altitude, mod_str1, 
            mod_str2, mod_str3, mod_str4, str_dayyear, oneWord, break_str, 
            stream1, stream, str_previous_day, str_f10year;
    std::string model_year, model_month, model_day, model_hour, model_minute, 
            model_second, r, s, t, u, v, w, x, y, z, search, line, 
            line_stream1, str_convert;
    std::stringstream model_output, model_utc1, model_utc2, line_stream;

    //RETRIEVE LATITUDE, LONGITUDE AND ALTITUDE
    model_output << std::fixed << std::setprecision(15) << std::setw(18)
                  << state->geodetic.lat << " " << std::setw(18)
                  << state->geodetic.lon << " " << std::setw(18)
                  << state->geodetic.alt << " " << std::setw(18);
    model_output >> model_latitude >> model_longitude >> model_altitude;
    if (model_latitude.length() > 8) {
        std::string str_new = model_latitude.substr (0, 8);
        model_latitude = str_new;
    }
    if (model_longitude.length() > 8) {
        std::string str_new = model_longitude.substr (0, 8);
        model_longitude = str_new;
    }
    if (model_altitude.length() > 9) {
        std::string str_new = model_altitude.substr (0, 8);
        model_altitude = str_new;
    }
    float flt_alt = std::stof(model_altitude);
    float flt_lat = std::stof(model_latitude);
    float flt_lon = std::stof(model_longitude);

    //CUMMULATIVE DAYS IN STANDARD / LEAP YEARS AND LIST OF LEAP YEARS
    std::map<int, int> cal_days = {{1,0,}, {2,31,}, {3,59,}, {4,90,}, {5,120,}, 
            {6,151,}, {7,181,}, {8,212,}, {9,243,}, {10,273,}, {11,304,}};
    std::map<int, int> leap_days = {{1,0,}, {2,31,}, {3,60,}, {4,91,}, {5,121,}, 
            {6,152,}, {7,182,}, {8,213,}, {9,244,}, {10,274,}, {11,305,}};
    std::vector<int> leap_yrs = { 1992, 2996, 2000, 2004, 2008, 2012, 2016, 
            2020 };
    
    //RETRIEVE UTC DATESTAMP & PARSE 
    model_utc1 << state->eci.epoch.str_UTC_datestamp();
    int count = 0;
    while(model_utc1 >> oneWord) { ++count;}
    model_utc2 << state->eci.epoch.str_UTC_datestamp();
    if (count == 3) {
        model_utc2 >> break_str >> mod_str3 >> mod_str4;
        unsigned slash1 = break_str.find_first_of("/"); 
        mod_str1 = break_str.substr (0, slash1);
        mod_str2 = break_str.substr (slash1 + 1, 
                (break_str.length() - mod_str1.length()));
    }
    else { 
        model_utc2 >> mod_str1 >> mod_str2 >> mod_str3 >> mod_str4;
    }
    int len1 = mod_str1.length();
    int len2 = mod_str2.length();
    int len3 = mod_str3.length();
    int len4 = mod_str4.length();   
    int int_year, int_month, int_day, int_dayyear, int_previous_day, int_f10year;
    model_year = mod_str2.substr (len2 - 5, 4);
    int_year = std::stoi( model_year );
    if (len2 == 7) {
        model_month = mod_str2.substr (0,1);
        int_month = std::stoi( model_month );
    }
    else {
        model_month = mod_str2.substr (0,2);
        int_month = std::stoi( model_month );
    }
    if (len1 == 2) {
        model_day = mod_str1.substr (0,1);
        int_day = std::stoi( model_day );
    }
    else {
        model_day = mod_str1.substr (0,2);
        int_day = std::stoi( model_day );
    };
    unsigned first = mod_str3.find_first_of(":");
    unsigned last = mod_str3.find_last_of(":");
    model_hour = mod_str3.substr(0,first);
    model_minute = mod_str3.substr (first + 1,last-first - 1);
    model_second = mod_str3.substr (last + 1,6);
    int int_hour = std::stoi( model_hour );
    int int_minute = std::stoi( model_minute );
    int int_sec = std::stoi( model_second ) + (int_minute * 60) 
            + (int_hour * 3600);
    model_second = std::to_string(int_sec);

    //CHECK IF LEAP YEAR AND CALCULATE DAY OF YEAR
    std::map<int,int>::iterator it1;
    std::map<int,int>::iterator it2;
    if (std::find(leap_yrs.begin(), leap_yrs.end(), 
            int_year) == leap_yrs.end()) {
        // Look up month in cal_days then add day of month for day of year
        it1 = cal_days.find(int_month);
        if (it1 != cal_days.end()) {
            int_dayyear = it1->second + int_day;
            if(int_dayyear > 1) {
                int_f10year = int_year;
                int_previous_day = int_dayyear - 1;
            }
            else {
                int_f10year = int_year - 1;
                int_previous_day = 365;
            }
            str_dayyear = std::to_string(int_dayyear);
            str_previous_day = std::to_string(int_previous_day);
            str_f10year = std::to_string(int_f10year);
        }
        else {
            std::cout << "Month not found in cal_days.";
            abort();
        }
    }
    else {
        // Look up month in leap_days then add the day of month to get 
        //...day of year (int_dayyear) and previous day (int_previous_day)
        it2 = leap_days.find(int_month);
        if (it2 != leap_days.end()) {
            int_dayyear = it2->second + int_day;
            if(int_dayyear > 1) {
                int_f10year = int_year;
                int_previous_day = int_dayyear - 1;
            }
            else {
                int_f10year = int_year - 1;
                int_previous_day = 366;
            }
            str_dayyear = std::to_string(int_dayyear);
            str_previous_day = std::to_string(int_previous_day);
            str_f10year = std::to_string(int_f10year);
        }
        else {
            std::cout << "Month not found in leap_days.";
            abort();
        }
    }

    //RETRIEVE F10.7 & F10.7A SOLAR FLUX INDEX VALUES FROM FILE:
    std::ifstream inFileF107;
    inFileF107.open("/Users/johnkeeling/Desktop/Astrophysics_MSc/PHAS0062"
            "_research_project/hawke_files/msis-model_c/DATA/SOLFSMY.TXT");
    std::string F107_value, F107A_value, w1, w2, w3;

    if (str_previous_day.length() == 3) {
        search = str_f10year + " " + str_previous_day; 
    }
    else if (str_previous_day.length() == 2) {
        search = str_f10year + "  " + str_previous_day; 
    }
    else {
        search = str_f10year + "   " + str_previous_day;
    }
    if(!inFileF107){
    std::cout << "Unable to open file" << std::endl;
    exit(1);
    }
    int pos;
    while(inFileF107.good()) {
        getline(inFileF107,line);
        pos=line.find(search);
        if(pos!=std::string::npos) {
                line_stream << line;
                line_stream >> w1 >> w2 >> w3 >> F107_value >> F107A_value;
            }     
    }

    //RETRIEVE AP GEOMAGNETIC INDEX VALUES FROM FILE:
    std::ifstream inFileAp;
    inFileAp.open("/Users/johnkeeling/Desktop/Astrophysics_MSc/"
            "PHAS0062_research_project/hawke_files/msis-model_c/DATA/apindex");
    std::string Ap_value, str_ap_date, Ap1, Ap2, Ap3, Ap4, Ap5, 
            Ap6, Ap7, Ap8;

    //--CREATE SEARCH STRING (yymmdd):
    if (model_month.length() == 1) {
        if(model_day.length() == 1) {
            str_ap_date = model_year.substr(2,2) + "0" 
                    + model_month + "0" + model_day;
        }
        else {
            str_ap_date = model_year.substr(2,2) + "0" 
                    + model_month + model_day;
        }
    }
    else {
        if(model_day.length() == 1) {
            str_ap_date = model_year.substr(2,2) 
                    + model_month + "0" + model_day;
        }
        else {
            str_ap_date = model_year.substr(2,2) + model_month + model_day;
        }
    }
    while(getline(inFileAp, line)) {
        if (line.find(str_ap_date, 0) != std::string::npos) {
            if (line.substr (0,6) == str_ap_date) {
                line_stream1 = line.substr (31,24);
            }
        }
    }

    //--FIND AVERAGE AP VALUE
    Ap1 = line_stream1.substr (0,3);
    Ap2 = line_stream1.substr (3,3);
    Ap3 = line_stream1.substr (6,3);
    Ap4 = line_stream1.substr (9,3);
    Ap5 = line_stream1.substr (12,3);
    Ap6 = line_stream1.substr (15,3);
    Ap7 = line_stream1.substr (18,3);
    Ap8 = line_stream1.substr (21,3);
    int int1 = std::stoi(Ap1);
    int int2 = std::stoi(Ap2);
    int int3 = std::stoi(Ap3);
    int int4 = std::stoi(Ap4);
    int int5 = std::stoi(Ap5);
    int int6 = std::stoi(Ap6);
    int int7 = std::stoi(Ap7);
    int int8 = std::stoi(Ap8);
    int int_Ap = round(float(int1 + int2 + int3 + int4 + int5 
            + int6 + int7 + int8) / 8);
    Ap_value = std::to_string(int_Ap);
    
    //COMMAND LINE INSTRUCTIONS TO RUN NRLMSISE00:
    std::string cmd = std::string("/Users/johnkeeling/Desktop/Astrophysics_MSc/"
            "PHAS0062_research_project/hawke_files/MSIS-model_c/"
            "nrlmsise_test01") 
            + std::string(" " + str_dayyear + " "  + model_year + " " 
            + model_second + "  " + model_altitude + "  " + model_latitude 
            + "  " + model_longitude + "  " + "0" + "  " + F107_value + "  " 
            + F107A_value + "  " + Ap_value);
    chdir("/Users/johnkeeling/Desktop/Astrophysics_MSc/"
            "PHAS0062_research_project/hawke_files/MSIS-model_c");  
    FILE *command=popen(cmd.c_str(), "r");
    char result[24]={0x0};
    std::string str_res;
    while (fgets(result, sizeof(result), command) !=NULL)
    pclose(command);
    chdir("/Users/johnkeeling/Desktop/Astrophysics_MSc/"
            "PHAS0062_research_project/hawke_files/odl20lite/bin");
    std::string str_start, str_end;

    //REFORMAT DENSITY VALUE TO MAKE IT CPP COMPATIBLE
    str_res = result;
    double rho;
    if (str_res == "inf" || str_res.substr(8,1) != "e") {
        rho = 1.000E-13;
        std::cout << "1.0E-16 substituted for infinite density value "
                "returned by nrlmsise." << std::endl;
    }
    else {
        unsigned slash2 = str_res.find_first_of("e"); 
        str_start = str_res.substr(0, slash2);
        str_end = str_res.substr(slash2 + 1, 3);
        //Conversion from gm/cm-3 to kg/m-3:
        int int_convert = std::stoi(str_end) + 3;
        str_convert = std::to_string(int_convert);
        str_res = str_start + "E" + str_convert;
        rho = std::stod(str_res); 
    }
    
    return rho;
};
