/*

Copyright (c) 2005-2016, University of Oxford.
All rights reserved.

University of Oxford means the Chancellor, Masters and Scholars of the
University of Oxford, having an administrative office at Wellington
Square, Oxford OX1 2JD, UK.

This file is part of Aboria.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
 * Neither the name of the University of Oxford nor the names of its
   contributors may be used to endorse or promote products derived from this
   software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/



#ifndef SYMBOLIC_DETAIL_H_
#define SYMBOLIC_DETAIL_H_

#include "Vector.h"


#include <boost/mpl/bool.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/mpl/equal.hpp>
#include <boost/proto/core.hpp>
#include <boost/proto/context.hpp>
#include <boost/proto/traits.hpp>
#include <boost/proto/transform.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/type_traits/ice.hpp>
#include <type_traits>
#include <tuple>
#include <map>

namespace mpl = boost::mpl;
namespace proto = boost::proto;
using proto::_;
using proto::N;

namespace Aboria {

    template<unsigned int I, typename P>
    struct Label; 

    namespace detail {

        template<typename Expr>
        struct SymbolicExpr;

        template<typename Expr>
        struct LabelExpr;

        template<typename Expr>
        struct GeometryExpr;


        ////////////////////////
        /// Terminal Classes ///
        ////////////////////////

        template<typename I, typename P>
        struct label {
            typedef P particles_type;
            typedef I depth;

            label(P& p):p(p) {}
            P& get_particles() const { return p; }

            P& p;
        };


        template<typename T>
        struct symbolic {
            typedef T variable_type;
            typedef typename T::value_type value_type;

            std::vector<value_type>& get_buffer(void *address) { return map[address]; }

            std::map<void *,std::vector<value_type> > map;
        };

        template <typename T>
        struct accumulate {
            typedef T functor_type;
            typedef typename T::result_type init_type;
            accumulate():init(0) {};
            accumulate(const T& functor):functor(functor) {};
            void set_init(const init_type& arg) {
                init = arg;
            }
            T functor;
            init_type init;
        };

        template <typename T>
        struct vector {
            typedef Vector<T,3> result_type;

            result_type operator()(const T arg1,const T arg2,const T arg3) const {
                return result_type(arg1,arg2,arg3);
            }
        };

        struct dx {};

        struct normal {
            typedef std::mt19937 generator_type;
            double operator()() {
                return normal(generator);
            }
            generator_type generator;
            std::normal_distribution<double> normal;


        };

        template <typename T>
        struct geometry {
            typedef T result_type;

            template <typename... A>
            result_type operator()(const A&... args) const {
                return T(args...);
            }
        };


        template <typename T>
        struct geometries {
            typedef T result_type;

            //result_type operator()(const Vect3d& centre, const double radius, const bool in) const
            template <typename... A>
            result_type operator()(const A&... args) const {
                return T(args...);
            }
        };




        ////////////////
        /// Grammars ///
        ////////////////


        // A grammar which matches all the assignment operators,
        // so we can easily disable them.
        struct AssignOps
            : proto::switch_<struct AssignOpsCases>
        {};

        // Here are the cases used by the switch_ above.
        struct AssignOpsCases
        {
            template<typename Tag, int D = 0> struct case_  : proto::not_<_> {};

            template<int D> struct case_< proto::tag::plus_assign, D >         : _ {};
            template<int D> struct case_< proto::tag::minus_assign, D >        : _ {};
            template<int D> struct case_< proto::tag::multiplies_assign, D >   : _ {};
            template<int D> struct case_< proto::tag::divides_assign, D >      : _ {};
            template<int D> struct case_< proto::tag::modulus_assign, D >      : _ {};
            template<int D> struct case_< proto::tag::shift_left_assign, D >   : _ {};
            template<int D> struct case_< proto::tag::shift_right_assign, D >  : _ {};
            template<int D> struct case_< proto::tag::bitwise_and_assign, D >  : _ {};
            template<int D> struct case_< proto::tag::bitwise_or_assign, D >   : _ {};
            template<int D> struct case_< proto::tag::bitwise_xor_assign, D >  : _ {};
        };


        struct SymbolicGrammar
            : proto::or_<
                proto::terminal<_>
                , proto::and_<
                    proto::nary_expr<_, proto::vararg<SymbolicGrammar> >
                    , proto::not_<AssignOps>
                >
              >

        {};

        struct get_particles: proto::callable {
            template<typename Sig>
                struct result;

            template<typename This, typename LabelType>
                struct result<This(const LabelType&)>
                : boost::add_reference<typename LabelType::particles_type>
                {};

            template<typename LabelType>
                typename LabelType::particles_type &operator()(const LabelType& label) {
                    return label.get_particles();
                }
        };



        struct LabelGrammar
            : proto::when< proto::terminal<label<_,_> >
              , get_particles(proto::_value) >
        {};

        struct SubscriptGrammar
            : proto::when< proto::terminal<label<_,_> >
              , proto::_value >
        {};


        struct GeometryGrammar
            : proto::function< proto::terminal< geometry<_> >, SymbolicGrammar, SymbolicGrammar, SymbolicGrammar>
        {}; 

        struct GeometriesGrammar
            : proto::function< proto::terminal< geometries<_> >, LabelGrammar, SymbolicGrammar>
        {}; 


        //TODO: should make a "conditional" grammar to match 2nd arguement
        struct AccumulateGrammar
            : proto::function< proto::terminal< accumulate<_> >, LabelGrammar, SymbolicGrammar, SymbolicGrammar>
        {}; 


        struct AccumulateInitGrammar
            : proto::function< proto::terminal< accumulate<_> >, LabelGrammar, SymbolicGrammar, SymbolicGrammar, SymbolicGrammar>
        {}; 


        ////////////////
        /// Contexts ///
        ////////////////


        template<typename ParticlesType>
        struct ParticleCtx;

        // Here is an evaluation context that indexes into a lazy vector
        // expression, and combines the result.
        template<typename ParticlesType1, typename ParticlesType2>
        struct TwoParticleCtx {
            TwoParticleCtx(const Vect3d& dx, const typename ParticlesType1::value_type& particle1, const typename ParticlesType2::value_type& particle2)
                    : dx_(dx),particle1_(particle1),particle2_(particle2)
            {}

            template<
                typename Expr
                // defaulted template parameters, so we can
                // specialize on the expressions that need
                // special handling.
                , typename Tag = typename proto::tag_of<Expr>::type
                , typename Enable = void
            >
            struct eval: proto::default_eval<Expr, TwoParticleCtx const>
            {};


            template<typename Expr>
            struct eval<Expr, proto::tag::terminal,
            typename boost::enable_if<proto::matches<Expr, proto::terminal<symbolic<_> > > >::type> {
                typedef typename proto::result_of::value<Expr>::type::variable_type variable_type;
                typedef const typename variable_type::value_type& result_type;

                result_type operator ()(Expr &expr, TwoParticleCtx const &ctx) const {
                    return get<variable_type>(ctx.particle1_);
                }
            };

            // Handle subscripted expressions here...
            template<typename Expr>
            struct eval<Expr, proto::tag::subscript,
                typename boost::enable_if<proto::matches<typename proto::result_of::child_c<Expr,1>::type,SubscriptGrammar> >::type 
            > {

                typedef typename proto::result_of::child_c<Expr, 1>::type subscript_type;

                BOOST_MPL_ASSERT(( proto::matches< subscript_type, SubscriptGrammar > ));

                typedef typename boost::result_of<SubscriptGrammar(subscript_type)>::type result_of_subscript_grammar;
                typedef typename std::remove_reference<result_of_subscript_grammar>::type::depth subscript_depth;

                BOOST_MPL_ASSERT_RELATION( subscript_depth::value , < , 2);

                typedef typename proto::result_of::child_c<Expr,0>::type expr_to_subscript;
                typedef typename mpl::vector<ParticlesType1,ParticlesType2> ParticlesTypes;
                typedef typename mpl::at<ParticlesTypes,subscript_depth>::type ParticlesType;

                typedef typename proto::result_of::eval<expr_to_subscript const, ParticleCtx<ParticlesType> const>::type result_type;

                result_type operator ()(Expr &expr, TwoParticleCtx const &ctx) const {
                    auto particles = std::tie(ctx.particle1_,ctx.particle2_);
                    ParticleCtx<ParticlesType> const single_ctx(std::get<subscript_depth::value>(particles));
                    return proto::eval(proto::child_c<0>(expr), single_ctx);
                            //return proto::child_c<0>(expr).eval<ParticlesType1>(ctx.particle1_);
                }
            };


            // Handle dx terminals here...
            template<typename Expr>
            struct eval<Expr, proto::tag::terminal, 
                typename boost::enable_if<proto::matches<Expr, proto::terminal<dx> > >::type 
            > {
                typedef const Vect3d& result_type;

                result_type operator ()(Expr &expr, TwoParticleCtx const &ctx) const {
                    return ctx.dx_;
                }
            };

            // Handle normal terminals here...
            template<typename Expr>
                struct eval<Expr, proto::tag::terminal,
                typename boost::enable_if<proto::matches<Expr, proto::terminal<normal> > >::type 
                    >
                    {
                        typedef double result_type;

                        result_type operator ()(Expr &expr, TwoParticleCtx const &ctx) const
                        {
                            return const_cast<typename ParticlesType1::value_type&>(ctx.particle1_).rand_normal();
                        }
                    };

            const typename ParticlesType1::value_type& particle1_;
            const typename ParticlesType2::value_type& particle2_;
            const Vect3d& dx_;

        };

        struct ConstantCtx {

            ConstantCtx() {}

            template<
                typename Expr
                , typename Tag = typename proto::tag_of<Expr>::type
                , typename Enable = void
                >
                struct eval: proto::default_eval<Expr, ConstantCtx const>
                {};


            // Handle normal terminals here...
            template<typename Expr>
                struct eval<Expr, proto::tag::terminal,
                typename boost::enable_if<proto::matches<Expr, proto::terminal<normal> > >::type 
                    >
                    {
                        typedef double result_type;

                        result_type operator ()(Expr &expr, ConstantCtx const &ctx) const
                        {
                            return proto::value(expr)();
                        }
                    };


            // Handle sums here...
            template<typename Expr>
                struct eval<Expr, proto::tag::function,
                typename boost::enable_if<proto::matches<Expr,AccumulateGrammar> >::type 
                    > {
                        typedef typename proto::result_of::child_c<Expr,0>::type child0_type;
                        typedef typename proto::result_of::child_c<Expr,1>::type child1_type;
                        typedef typename proto::result_of::child_c<Expr,2>::type child2_type;
                        typedef typename proto::result_of::child_c<Expr,3>::type child3_type;

                        typedef typename boost::result_of<LabelGrammar(child1_type)>::type particles_type_ref;
                        typedef typename std::remove_reference<particles_type_ref>::type particles_type;
                        typedef typename proto::result_of::value<child0_type>::type functor_terminal_type;
                        typedef typename functor_terminal_type::functor_type functor_type;
                        typedef typename functor_type::result_type result_type;

                        result_type operator ()(Expr &expr, ConstantCtx const &ctx) const
                        {
                            particles_type_ref particlesa = LabelGrammar()(proto::child_c<1>(expr));
                            result_type sum = proto::value(proto::child_c<0>(expr)).init;
                            functor_type accumulate = proto::value(proto::child_c<0>(expr)).functor;
                            for (auto i: particlesa) {
                                ParticleCtx<particles_type> ctx(i);
                                if (proto::eval(proto::child_c<2>(expr),ctx)) {
                                    sum = accumulate(sum,proto::eval(proto::child_c<3>(expr),ctx));
                                }
                            }
                            return sum;
                        }
                    };

        };



        // Here is an evaluation context that indexes into a lazy vector
        // expression, and combines the result.
        template<typename ParticlesType>
        struct ParticleCtx {
                ParticleCtx(const typename ParticlesType::value_type& particle)
                    : particle_(particle)
                {}

                template<
                    typename Expr
                    // defaulted template parameters, so we can
                    // specialize on the expressions that need
                    // special handling.
                    , typename Tag = typename proto::tag_of<Expr>::type
                    , typename Enable = void
                    >
                    struct eval: proto::default_eval<Expr, ParticleCtx const>
                    {};



                // Handle unlabeled vector terminals here...
                template<typename Expr>
                    struct eval<Expr, proto::tag::terminal, 
                    typename boost::enable_if<proto::matches<Expr, proto::terminal<symbolic<_> > > >::type 
                        > 
                        {
                            typedef typename proto::result_of::value<Expr>::type::variable_type variable_type;
                            typedef const typename variable_type::value_type& result_type;

                            result_type operator ()(Expr &expr, ParticleCtx const &ctx) const
                            {
                                return get<variable_type>(ctx.particle_);
                            }
                        };

                // Handle normal terminals here...
                template<typename Expr>
                    struct eval<Expr, proto::tag::terminal,
                    typename boost::enable_if<proto::matches<Expr, proto::terminal<normal> > >::type 
                        >
                        {
                            typedef double result_type;

                            result_type operator ()(Expr &expr, ParticleCtx const &ctx) const
                            {
                                //TODO: get better (parallel) random number generator
                                return const_cast<typename ParticlesType::value_type&>(ctx.particle_).rand_normal();
                            }
                        };

                // Handle subscripts here...
                template<typename Expr>
                    struct eval<Expr, proto::tag::subscript,
                    typename boost::enable_if<proto::matches<typename proto::result_of::child_c<Expr,1>::type,SubscriptGrammar> >::type 
                        >
                        {
                            typedef typename proto::result_of::child_c<Expr, 1>::type subscript_type;
                            typedef typename boost::result_of<SubscriptGrammar(subscript_type)>::type result_of_subscript_grammar;
                            typedef typename std::remove_reference<result_of_subscript_grammar>::type::depth subscript_depth;

                            BOOST_MPL_ASSERT_RELATION( subscript_depth::value, == , 0 );

                            typedef typename proto::result_of::child_c<Expr, 0>::type ExprToSubscript;

                            typedef typename proto::result_of::eval<ExprToSubscript const, ParticleCtx<ParticlesType> const>::type result_type;

                            result_type operator ()(Expr &expr, ParticleCtx const &ctx) const
                            {
                                return proto::eval(proto::child_c<0>(expr),ctx);
                            }
                        };

                template<typename Expr>
                    struct eval<Expr, proto::tag::bitwise_or,
                    typename boost::enable_if<proto::matches<typename proto::result_of::child_c<Expr,1>::type,GeometriesGrammar> >::type 
                        >
                        {

                            typedef Vect3d result_type;

                            result_type operator ()(Expr &expr, ParticleCtx const &ctx) const
                            {
                                typedef typename proto::result_of::child_c<Expr,1>::type geometry_expr_type;
                                typedef typename proto::result_of::child_c<geometry_expr_type,0>::type geometry_functor_terminal_type;
                                typedef typename proto::result_of::value<geometry_functor_terminal_type>::type geometry_functor_type;
                                typedef typename geometry_functor_type::result_type geometry_type;
                                geometry_expr_type geometry_expr = proto::child_c<1>(expr);
                                Vect3d vector = proto::eval(proto::child_c<0>(expr),ctx);
                                typedef typename proto::result_of::child_c<geometry_expr_type,1>::type label_expr_type;
                                typedef typename proto::result_of::child_c<geometry_expr_type,2>::type arg1_expr_type;
                                typedef typename boost::result_of<LabelGrammar(label_expr_type)>::type particles_type_ref;
                                typedef typename std::remove_reference<particles_type_ref>::type particles_type;
                                particles_type_ref particlesb = LabelGrammar()(proto::child_c<1>(geometry_expr));
                                arg1_expr_type arg1_expr = proto::child_c<2>(geometry_expr);
                                //std::cout << "doing reflect for particle "<<get<id>(ctx.particle_)<<std::endl;
                                bool keep_going = true;
                                while (keep_going) {
                                    keep_going = false;	
                                    //std::cout << "searching around position = "<<vector + get<position>(ctx.particle_)<<std::endl;
                                    for (auto i: particlesb.get_neighbours(vector + get<position>(ctx.particle_))) {
                                        //std::cout << "doing neighbour "<<get<id>(std::get<0>(i))<<std::endl;
                                        TwoParticleCtx<ParticlesType,particles_type> ctx2(std::get<1>(i),ctx.particle_,std::get<0>(i));
                                        geometry_type geometry(vector-ctx2.dx_,proto::eval(arg1_expr,ctx2),true);
                                        //std::cout <<"result of evaluating geometry is "<<geometry<<std::endl;
                                        if (reflect_once(Vect3d(0,0,0),vector,geometry)) 
                                        {
                                            keep_going = true;
                                        }
                                    }
                                }
                                return vector;
                            }
                        };


                template<typename Expr>
                    struct eval<Expr, proto::tag::function,
                    typename boost::enable_if<proto::matches<Expr,AccumulateGrammar> >::type 
                        >
                        {
                            typedef typename proto::result_of::child_c<Expr,0>::type child0_type;
                            typedef typename proto::result_of::child_c<Expr,1>::type child1_type;
                            typedef typename proto::result_of::child_c<Expr,2>::type child2_type;
                            typedef typename proto::result_of::child_c<Expr,3>::type child3_type;

                            typedef typename boost::result_of<LabelGrammar(child1_type)>::type particles_type_ref;
                            typedef typename std::remove_reference<particles_type_ref>::type particles_type;
                            typedef typename proto::result_of::value<child0_type>::type functor_terminal_type;
                            typedef typename functor_terminal_type::functor_type functor_type;
                            typedef typename functor_type::result_type result_type;


                            result_type operator ()(Expr &expr, ParticleCtx const &ctx) const
                            {
                                particles_type_ref particlesb = LabelGrammar()(proto::child_c<1>(expr));
                                result_type sum = proto::value(proto::child_c<0>(expr)).init;
                                functor_type accumulate = proto::value(proto::child_c<0>(expr)).functor;
                                for (auto i: particlesb.get_neighbours(get<position>(ctx.particle_))) {
                                    TwoParticleCtx<ParticlesType,particles_type> ctx2(std::get<1>(i),ctx.particle_,std::get<0>(i));
                                    if (proto::eval(proto::child_c<2>(expr),ctx2)) {
                                        sum = accumulate(sum,proto::eval(proto::child_c<3>(expr),ctx2));
                                    }
                                }
                                return sum;
                            }
                        };

                const typename ParticlesType::value_type& particle_;
            };



        ////////////////
        /// Domains  ///
        ////////////////



        // Tell proto how to generate expressions in the SymbolicDomain
        struct SymbolicDomain
            : proto::domain<proto::generator<SymbolicExpr>, SymbolicGrammar > {};

        // Declare that phoenix_domain is a sub-domain of spirit_domain
        struct LabelDomain 
            : proto::domain<proto::generator<LabelExpr>, LabelGrammar, SymbolicDomain> {};

        struct GeometryDomain 
            : proto::domain<proto::generator<GeometryExpr>, GeometryGrammar> {};


        ///////////////////
        /// Expressions ///
        ///////////////////

        template<typename Expr>
        struct SymbolicExpr
            : proto::extends<Expr, SymbolicExpr<Expr>, SymbolicDomain> {
                explicit SymbolicExpr(Expr const &expr)
                    : proto::extends<Expr, SymbolicExpr<Expr>, SymbolicDomain>(expr)
                {}

                // Use the ParticleCtx to implement subscripting
                // of a Symbolic expression tree.
                template<typename ParticleType>
                    typename proto::result_of::eval<Expr const, ParticleCtx<ParticleType> const>::type
                    eval( const typename ParticleType::value_type& particle) const
                    {
                        ParticleCtx<ParticleType> const ctx(particle);
                        return proto::eval(*this, ctx);
                    }
                template<typename ParticleType1, typename ParticleType2>
                    typename proto::result_of::eval<Expr const, TwoParticleCtx<ParticleType1,ParticleType2> const>::type
                    eval( const Vect3d& dx, const typename ParticleType1::value_type& particle1,  const typename ParticleType2::value_type& particle2) const
                    {
                        TwoParticleCtx<ParticleType1,ParticleType2> const ctx(dx, particle1, particle2);
                        return proto::eval(*this, ctx);
                    }

            };


        template<typename Expr>
        struct GeometryExpr: proto::extends<Expr, GeometryExpr<Expr>, GeometryDomain> {
            explicit GeometryExpr(Expr const &expr)
                : proto::extends<Expr, GeometryExpr<Expr>, GeometryDomain>(expr)
            {}
        };


        template<typename Expr>
        struct LabelExpr: proto::extends<Expr, LabelExpr<Expr>, LabelDomain> {
            explicit LabelExpr(Expr const &expr)
                : proto::extends<Expr, LabelExpr<Expr>, LabelDomain>(expr)
            {}


            BOOST_PROTO_EXTENDS_USING_ASSIGN(LabelExpr)
        };


        template<unsigned int I, typename P>
        struct Label 
            : LabelExpr<typename proto::terminal<label<mpl::int_<I>,P > >::type> {

                typedef typename proto::terminal<label<mpl::int_<I>,P > >::type expr_type;
                typedef label<mpl::int_<I>,P > data_type;

                explicit Label(P& p)
                    : LabelExpr<expr_type>( expr_type::make(data_type(p)))
                {}


                //BOOST_PROTO_EXTENDS_USING_ASSIGN(Label)
            };



        #define SUBSCRIPT_TYPE \
        boost::proto::exprns_::basic_expr< \
            boost::proto::tagns_::tag::subscript \
            , boost::proto::argsns_::list2< \
                Aboria::detail::SymbolicExpr<boost::proto::exprns_::expr< \
                    boost::proto::tagns_::tag::terminal \
                    , boost::proto::argsns_::term<Aboria::detail::symbolic<VariableType> > \
                , 0l> >& \
            , Aboria::Label<0u, ParticlesType >& \
            > \
        , 2l \
        >


        // TODO: this seems a messy way to define a symbol subscripted by a label. might
        // be better to put a subscript operator in the Symbol class?
        // for: cleaner
        // against: messes up the grammer (already existing subscript expression). 
        // Could have Symbol *not* be an expression, but then can't assume labels 
        // within expressions anymore....
        template<typename VariableType, typename ParticlesType>
        struct SymbolicExpr<SUBSCRIPT_TYPE>
            : proto::extends<SUBSCRIPT_TYPE, SymbolicExpr<SUBSCRIPT_TYPE>, SymbolicDomain> {
                typedef SUBSCRIPT_TYPE Expr;

        #undef SUBSCRIPT_TYPE

                explicit SymbolicExpr(Expr const &expr)
                    : proto::extends<Expr, SymbolicExpr<Expr>, SymbolicDomain>(expr),
                    msymbol(proto::value(proto::child_c<0>(expr))),
                    mlabel(proto::value(proto::child_c<1>(expr))) {}

                // Use the ParticleCtx to implement subscripting
                // of a Symbolic expression tree.
                template<typename ParticleType>
                typename proto::result_of::eval<Expr const, ParticleCtx<ParticleType> const>::type
                eval( const typename ParticleType::value_type& particle) const {
                    ParticleCtx<ParticleType> const ctx(particle);
                    return proto::eval(*this, ctx);
                }
                template<typename ParticleType1, typename ParticleType2>
                typename proto::result_of::eval<Expr const, TwoParticleCtx<ParticleType1,ParticleType2> const>::type
                eval( const Vect3d& dx, const typename ParticleType1::value_type& particle1,  const typename ParticleType2::value_type& particle2) const {
                    TwoParticleCtx<ParticleType1,ParticleType2> const ctx(dx, particle1, particle2);
                    return proto::eval(*this, ctx);
                }


                //mpl::for_each<mpl::range_c<int,1,label_type::number_of_particle_sets::value> > (this->name(proto::as_expr<SymbolicDomain>(expr,label))); \

                #define DEFINE_THE_OP(name,the_op) \
                template< typename ExprRight > \
                const SymbolicExpr &operator the_op (ExprRight const & expr) const { \
                    BOOST_MPL_ASSERT_NOT(( boost::is_same<VariableType,id > )); \
                    this->name(proto::as_expr<SymbolicDomain>(expr)); \
                    return *this; \
                } \

                DEFINE_THE_OP(assign,=)
                DEFINE_THE_OP(increment,+=)
                DEFINE_THE_OP(decrement,-=)
                DEFINE_THE_OP(divide,/=)
                DEFINE_THE_OP(multiply,*=)

                private:

                label<mpl::int_<0>,ParticlesType> &mlabel;
                symbolic<VariableType> &msymbol;

                void post() const {
                    typedef typename VariableType::value_type value_type;

                    ParticlesType& particles = mlabel.get_particles();
                    std::vector<value_type>& buffer = msymbol.get_buffer(&particles);

                    for (int i=0; i<particles.size(); i++) {
                        set<VariableType>(particles[i],buffer[i]);	
                    }

                    if (boost::is_same<VariableType,position>::value) {
                        particles.update_positions();
                    }

                    if (boost::is_same<VariableType,alive>::value) {
                        particles.delete_particles();
                    }
                }

                template< typename Expr>
                const SymbolicExpr &assign(Expr const & expr) const {
                    typedef typename VariableType::value_type value_type;

                    ParticlesType& particles = mlabel.get_particles();
                    std::vector<value_type>& buffer = msymbol.get_buffer(&particles);

                    buffer.resize(particles.size());

                    //TODO: if vector to assign to does not exist in depth > 0, then don't need buffer
                    for (int i=0; i<particles.size(); i++) {
                        buffer[i] =  expr.template eval<ParticlesType>(particles[i]);	
                    }

                    post();

                    return *this;
                }


                #define DO_THE_OP(name,the_op) \
                template< typename Expr > \
                const SymbolicExpr & name (Expr const & expr) const \
                {                                                \
                    typedef typename VariableType::value_type value_type; \
                    ParticlesType& particles = mlabel.get_particles(); \
                    std::vector<value_type>& buffer = msymbol.get_buffer(&particles); \
                    buffer.resize(particles.size()); \
                    for (int i=0; i<particles.size(); i++) { \
                        buffer[i] = get<VariableType>(particles[i]) the_op expr.template eval<ParticlesType>(particles[i]);	\
                    } \
                    post(); \
                    return *this; \
                } \

                DO_THE_OP(increment,+)
                DO_THE_OP(multiply,*)
                DO_THE_OP(divide,/)
                DO_THE_OP(decrement,-)
        };

    }
}



#endif /* SYMBOLIC_H_ */
