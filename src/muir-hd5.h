#ifndef MUIR_HD5_H
#define MUIR_HD5_H
//
// C++ Declaration: hd5-io
//
// Description: Functions for manipulating HD5 Files
//
//
// Author: Beau V.C. Bellamy <bvbellamy@arsc.edu>
//         Arctic Region Supercomputing Center
// 
//

#include "muir-types.h"

#include <H5Cpp.h>
#include <string>


class MuirHD5 : public H5::H5File
{
    public:
        MuirHD5(const std::string &filename_in, const unsigned int flags);
        virtual ~MuirHD5();

        const std::string &filename() const;

        float read_scalar_float(const H5std_string &dataset_name) const;

        void  write_scalar_uint(const H5std_string &dataset_name, unsigned int out);
        void  write_scalar_float(const H5std_string &dataset_name, float out);
        void  write_scalar_double(const H5std_string &dataset_name, double out);

        std::string read_string(const H5std_string &dataset_name) const;
        void        write_string(const H5std_string &dataset_name, const std::string &out);

        void write_1D_string(const H5std_string &dataset_name, const std::vector<std::string> &out);

        void read_2D_uint(const H5std_string &dataset_name, Muir2DArrayUI &in) const;
        void read_3D_uint(const H5std_string &dataset_name, Muir3DArrayUI &in) const;
        void read_4D_uint(const H5std_string &dataset_name, Muir4DArrayUI &in) const;

        void write_2D_uint(const H5std_string &dataset_name, const Muir2DArrayUI &out);
        void write_3D_uint(const H5std_string &dataset_name, const Muir3DArrayUI &out);
        void write_4D_uint(const H5std_string &dataset_name, const Muir4DArrayUI &out);

        void read_2D_float(const H5std_string &dataset_name, Muir2DArrayF &in) const;
        void read_3D_float(const H5std_string &dataset_name, Muir3DArrayF &in) const;
        void read_4D_float(const H5std_string &dataset_name, Muir4DArrayF &in) const;

        void write_2D_float(const H5std_string &dataset_name, const Muir2DArrayF &out);
        void write_3D_float(const H5std_string &dataset_name, const Muir3DArrayF &out);
        void write_4D_float(const H5std_string &dataset_name, const Muir4DArrayF &out);

        void read_2D_double(const H5std_string &dataset_name, Muir2DArrayD &in) const;
        void read_3D_double(const H5std_string &dataset_name, Muir3DArrayD &in) const;
        void read_4D_double(const H5std_string &dataset_name, Muir4DArrayD &in) const;

        void write_2D_double(const H5std_string &dataset_name, const Muir2DArrayD &out);
        void write_3D_double(const H5std_string &dataset_name, const Muir3DArrayD &out);
        void write_4D_double(const H5std_string &dataset_name, const Muir4DArrayD &out);

    private:
    // No copying
        MuirHD5(const MuirHD5 &in);
        MuirHD5& operator= (const MuirHD5 &right);

        //std::string _filename;
        //H5::H5File  _h5file;

};

#endif // #ifndef MUIR_HD5_H
