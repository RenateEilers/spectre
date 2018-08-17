#ifndef __Options__
#define __Options__

#include <initializer_list>
#include <map>
#include <vector>
#include <string>

namespace util {
    
    class Option {
    public:
        std::string name() { return _name; }
        
        // return true if the value was succesfully set
        virtual bool setValue(std::string v) = 0;
        
    protected:
        Option(std::string name) :
        _name(name)
        {}
        
        std::string _name;
    };
    
    class BooleanOption : public Option {
    public:
        BooleanOption(std::string name, bool defaultValue) :
        Option(name),
        _value(defaultValue)
        {}
        
        bool setValue(std::string v);
        
        bool getValue() { return _value; }
        
    protected:
        bool _value;
    };
    
    class StringOption : public Option {
    public:
        StringOption(std::string name, std::string defaultValue) :
        Option(name),
        _value(defaultValue)
        {}
        
        bool setValue(std::string v) { _value = v; return true; }
        
        std::string getValue() { return _value; }
        
    protected:
        std::string _value;
    };
    
    class MultiChoiceOption : public Option {
    public:
        MultiChoiceOption(std::string name, std::initializer_list<std::string> choices, std::string defaultValue) :
        Option(name),
        _value(defaultValue),
        _choices(choices)
        {}
        
        bool setValue(std::string v);
        
        std::string getValue() { return _value; }
        
    protected:
        std::string _value;
        
        std::vector<std::string> _choices;
    };
    
    class Configuration {
    public:
        Configuration() :
        _outputFile("output", ""),
        _timepoints("timepoints", false),
        _allOptions()
        {
            registerOption(&_outputFile);
            registerOption(&_timepoints);
        }
        
        bool setAllValues(int argc, char *argv[]);
        
        Option* getOption(std::string name);
        
        StringOption outputFile() { return _outputFile; }
        BooleanOption timepoints() { return _timepoints; }
        
        static Configuration instance() { return _instance; }
        
    protected:
        StringOption _outputFile;
        BooleanOption _timepoints;
        
        std::map<std::string, Option*> _allOptions;
        
        void registerOption(Option* o);
        
        static Configuration _instance;
    };
}

#endif

